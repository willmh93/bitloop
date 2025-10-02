#include <bitloop/core/record_manager.h>

BL_BEGIN_NS;


static bool is_h264(const AVCodec* c) { return c && c->id == AV_CODEC_ID_H264; }
static bool is_h265(const AVCodec* c) { return c && c->id == AV_CODEC_ID_HEVC; }
static bool h264_frame_size_supported(int w, int h)
{
    const int mb = ((w + 15) / 16) * ((h + 15) / 16);
    return mb <= 36864; // Level 5.2 macroblock bound (approx)
}

static bool g_want_hevc_10bit = false;
static enum AVPixelFormat SelectPixelFormat(AVCodecContext*, const enum AVPixelFormat* fmts)
{
    // Prefer 10-bit if requested, otherwise 8-bit 4:2:0
    if (g_want_hevc_10bit) {
        for (const enum AVPixelFormat* p = fmts; *p != AV_PIX_FMT_NONE; ++p)
            if (*p == AV_PIX_FMT_YUV420P10LE) return AV_PIX_FMT_YUV420P10LE;
    }
    for (const enum AVPixelFormat* p = fmts; *p != AV_PIX_FMT_NONE; ++p)
        if (*p == AV_PIX_FMT_YUV420P) return AV_PIX_FMT_YUV420P;

    // Fallback to the first offered format
    return fmts[0];
}

void FFmpegWorker::process(RecordManager* record_manager, VideoConfig video_config)
{
    config = video_config;

    if (!startRecording())
        return;

    while (true)
    {
        if (!record_manager->waitForWorkAvailable())
            break; // recording stopped, nothing left to do

        if (!record_manager->isBusy())
        {
            // If finalize was requested and no frame is pending, flush and exit
            if (record_manager->shouldFinalize() )
            {
                finalizeRecording();
                record_manager->clearFinalizeRequest();
                break;
            }
        }
        else
        {
            // Grab the pending frame
            bytebuf frame = record_manager->takePendingFrame();

            if (frame.empty())
            {
                // Shouldn't occur, but if it does, check we're still recording and skip frame
                if (!record_manager->isRecording()) break;
                continue;
            }

            encodeFrame(frame);

            // Tell record_manager we're ready for the next frame
            record_manager->markEncoderIdle();
        }

        if (record_manager->shouldFinalize())
        {
            finalizeRecording();
            record_manager->clearFinalizeRequest();
            break;
        }
    }
}

bool FFmpegWorker::startRecording()
{
    const char* filename = config.filename.c_str();
    trimmed_resolution = (config.resolution / 2) * 2;

    // Allocate format context
    if (avformat_alloc_output_context2(&format_context, nullptr, nullptr, filename) < 0)
    {
        blPrint() << "ERROR: avformat_alloc_output_context2(...)";
        return false;
    }

    // Find H.264 codec
    //const AVCodec* codec = avcodec_find_encoder(AV_CODEC_ID_H264);
    const AVCodec* codec = nullptr;

    if (config.encoding == VideoEncoding::x265)
        codec = avcodec_find_encoder(AV_CODEC_ID_HEVC);
    else
        codec = avcodec_find_encoder(AV_CODEC_ID_H264);

    // Avoid H.264 at 8K
    if (is_h264(codec) && !h264_frame_size_supported(trimmed_resolution.x, trimmed_resolution.y))
    {
        blPrint() << "ERROR: H.264 does not support resolution "
                  << trimmed_resolution.x << "x" << trimmed_resolution.y
                  << " at the selected level. Choose H.265 or reduce resolution.";
        return false;
    }

    if (!codec) { blPrint() << "ERROR: avcodec_find_encoder(...)"; return false; }

    // Create video stream
    stream = avformat_new_stream(format_context, codec);
    if (!stream) { blPrint() << "ERROR: avformat_new_stream(...)"; return false; }

    // Allocate codec context
    codec_context = avcodec_alloc_context3(codec);
    if (!codec_context) { blPrint() << "ERROR: avcodec_alloc_context3(...)"; return false; }

    // If muxer wants global headers, set flag
    if (format_context->oformat->flags & AVFMT_GLOBALHEADER)
        codec_context->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

    // Set codec parameters
    codec_context->bit_rate = config.bitrate;
    codec_context->width = trimmed_resolution.x;
    codec_context->height = trimmed_resolution.y;
    codec_context->time_base = { 1, config.fps };
    codec_context->framerate = { config.fps, 1 };
    codec_context->gop_size = config.fps * 2; // Group of pictures
    codec_context->max_b_frames = is_h265(codec) ? 6 : 3;;
    codec_context->pix_fmt = AV_PIX_FMT_YUV420P;

    codec_context->get_format = &SelectPixelFormat;
    ///const bool got_10bit = (codec_context->pix_fmt == AV_PIX_FMT_YUV420P10LE);

    // Set preset for quality/speed tradeoff
    if (is_h265(codec))
    {
        // Prefer 10-bit for quality (if requested and build supports it)
        av_opt_set(codec_context->priv_data, "profile", g_want_hevc_10bit ? "main10" : "main", 0);
        av_opt_set(codec_context->priv_data, "preset", "medium", 0);
    }
    else {
        // H.264 (8-bit typical unless you use a 10-bit x264 build)
        av_opt_set(codec_context->priv_data, "profile", "high", 0);
        av_opt_set(codec_context->priv_data, "preset", "veryslow", 0);
    }

    // VBV to constrain peaks (keeps muxers/players happy)
    av_opt_set_int(codec_context->priv_data, "maxrate", codec_context->bit_rate, 0);
    av_opt_set_int(codec_context->priv_data, "bufsize", codec_context->bit_rate / 2, 0);

    ///av_opt_set(codec_context->priv_data, "preset", "veryslow", 0);
    ///av_opt_set(codec_context->priv_data, "profile", "high", 0);

    // Open codec
    if (avcodec_open2(codec_context, codec, nullptr) < 0)
    {
        blPrint() << "ERROR: avcodec_open2(...)";
        return false;
    }

    // Associate codec parameters with stream
    if (avcodec_parameters_from_context(stream->codecpar, codec_context) < 0)
    {
        blPrint() << "ERROR: avcodec_parameters_from_context(...)";
        return false;
    }

    stream->time_base = codec_context->time_base;

    // Open output file
    if (!(format_context->oformat->flags & AVFMT_NOFILE)) {
        if (avio_open(&format_context->pb, filename, AVIO_FLAG_WRITE) < 0)
        {
            blPrint() << "ERROR: avio_open(...)";
            return false;
        }
    }

    // Write the stream header
    if (avformat_write_header(format_context, nullptr) < 0)
    {
        blPrint() << "ERROR: avformat_write_header(...)";
        return false;
    }

    // Allocate frame for YUV420P
    yuv_frame = av_frame_alloc();
    if (!yuv_frame) { blPrint() << "ERROR: av_frame_alloc()"; return false; }
    yuv_frame->format = codec_context->pix_fmt;
    yuv_frame->width = trimmed_resolution.x;
    yuv_frame->height = trimmed_resolution.y;
    if (av_frame_get_buffer(yuv_frame, 32) < 0) { blPrint() << "ERROR: av_frame_get_buffer()"; return false; }

    // Allocate frame for RGB
    rgb_frame = av_frame_alloc();
    if (!rgb_frame) { blPrint() << "ERROR: av_frame_alloc()"; return false; }

    rgb_frame->format = AV_PIX_FMT_RGBA;
    rgb_frame->width = config.resolution.x;
    rgb_frame->height = config.resolution.y;
    if (av_frame_get_buffer(rgb_frame, 32) < 0)
    {
        blPrint() << "ERROR: av_frame_get_buffer(...)";
        return false;
    }

    // Initialize scaling context
    sws_ctx = sws_getContext(
        config.resolution.x, config.resolution.y, AV_PIX_FMT_RGBA,           // src format
        codec_context->width, codec_context->height, codec_context->pix_fmt, // dest format
        SWS_BILINEAR, nullptr, nullptr, nullptr);

    if (!sws_ctx) { blPrint() << "ERROR: sws_getContext(...)"; return false; }

    // Allocate packet
    packet = av_packet_alloc();
    if (!packet) { blPrint() << "ERROR: av_packet_alloc()"; return false; }

    frame_index = 0;
    return true;
}

bool FFmpegWorker::encodeFrame(bytebuf& frame)
{
    av_frame_make_writable(rgb_frame);

    int src_line_size = config.resolution.x * 4;
    int targ_line_size = rgb_frame->linesize[0];

    uint8_t* data = frame.data();
    const uint8_t* src_row = nullptr;
    uint8_t* targ_row = nullptr;

    if (config.flip)
    {
        int targ_y = 0;
        for (int src_y = trimmed_resolution.y - 1; src_y >= 0; src_y--)
        {
            // Copy row
            src_row = data + (src_y * src_line_size);
            targ_row = rgb_frame->data[0] + (targ_y * targ_line_size);

            memcpy(targ_row, src_row, targ_line_size);

            targ_y++;
        }
    }
    else
    {
        for (int src_y = 0; src_y < trimmed_resolution.y; src_y++)
        {
            // Copy row
            src_row = data + (src_y * src_line_size);
            targ_row = rgb_frame->data[0] + (src_y * targ_line_size);

            memcpy(targ_row, src_row, targ_line_size);
        }
    }

    // Convert RGB frame to YUV420P
    sws_scale(sws_ctx,
        rgb_frame->data, rgb_frame->linesize,   // Source frame data
        0, config.resolution.y,                 // Source frame height
        yuv_frame->data, yuv_frame->linesize);  // Output frame data

    blPrint() << "<<Encoding Frame>> " << frame_index;
    yuv_frame->pts = frame_index++;

    if (avcodec_send_frame(codec_context, yuv_frame) < 0)
        return false;

    while (avcodec_receive_packet(codec_context, packet) == 0)
    {
        av_packet_rescale_ts(packet, codec_context->time_base, stream->time_base);
        packet->stream_index = stream->index;

        if (av_interleaved_write_frame(format_context, packet) < 0)
            return false;

        av_packet_unref(packet);
    }

    if (finalizing)
        finalizeRecording();

    return true;
}

bool FFmpegWorker::finalizeRecording()
{
    blPrint() << "<<Finalizing>>";

    // Flush the encoder
    avcodec_send_frame(codec_context, nullptr);
    while (avcodec_receive_packet(codec_context, packet) == 0)
    {
        av_packet_rescale_ts(packet, codec_context->time_base, stream->time_base);
        packet->stream_index = stream->index;
        av_interleaved_write_frame(format_context, packet);
        av_packet_unref(packet);
    }

    // Write trailer
    av_write_trailer(format_context);

    // Free resources
    sws_freeContext(sws_ctx);
    av_frame_free(&yuv_frame);
    av_frame_free(&rgb_frame);
    av_packet_free(&packet);
    avcodec_free_context(&codec_context);
    avio_close(format_context->pb);
    avformat_free_context(format_context);

    return true;
}

void RecordManager::requestFinalizeAfterNextFrame() // ask the ffmpeg worker to finalize after the next frame (or immediately if idle)
{
    finalize_requested.store(true, std::memory_order_release);

    // wake worker in case it's sleeping
    work_available_cond.notify_all();
}

bool RecordManager::startRecording(
    std::string filename,
    IVec2 resolution,
    int fps,
    int64_t bitrate,
    VideoEncoding encoding,
    bool flip)
{
    config.filename = filename;
    config.resolution = resolution;
    config.fps = fps;
    config.bitrate = bitrate;
    config.encoding = encoding;
    config.flip = flip;

    recording.store(true, std::memory_order_release);

    ffmpeg_worker_thread = std::thread(&FFmpegWorker::process, &ffmpeg_worker, this, config);

    blPrint() << "startRecording()";

    return true;
}

void RecordManager::finalizeRecording()
{
    requestFinalizeAfterNextFrame();

    // close worker thread
    if (ffmpeg_worker_thread.joinable())
        ffmpeg_worker_thread.join();

    recording.store(false, std::memory_order_release);
    encoder_busy.store(false, std::memory_order_release);

    clearFinalizeRequest();

    // wake any sim thread
    encoder_ready_cond.notify_all();

    blPrint() << "finalizeRecording()";
}

void RecordManager::waitUntilReadyForNewFrame()
{
    std::unique_lock<std::mutex> lock(encoder_ready_mutex);
    encoder_ready_cond.wait(lock, [this]
    {
        return !encoder_busy.load(std::memory_order_acquire) || !isRecording();
    });
}

bool RecordManager::encodeFrame(const uint8_t* data)
{
    if (!isRecording()) return false;

    // If encoder still busy with previous frame, do not block here
    if (encoder_busy.load(std::memory_order_acquire))
        return false;

    {
        std::lock_guard<std::mutex> lock(pending_mutex);
        pending_frame.resize(config.numBytes());
        std::memcpy(pending_frame.data(), data, config.numBytes());

        // Mark busy only after the buffer is fully populated
        encoder_busy.store(true, std::memory_order_release);
    }

    // Wake the FFmpeg worker, a new frame is ready to encode
    work_available_cond.notify_one();

    return true;
}

bool RecordManager::waitForWorkAvailable()
{
    std::unique_lock<std::mutex> lock(work_available_mutex);
    work_available_cond.wait(lock, [this]{
        return encoder_busy.load(std::memory_order_acquire)
            || finalize_requested.load(std::memory_order_acquire);
    });

    return true;
}

std::vector<uint8_t> RecordManager::takePendingFrame()
{
    std::lock_guard<std::mutex> lock(pending_mutex);
    std::vector<uint8_t> out;
    out.swap(pending_frame); // zero-copy handoff
    return out;
}

void RecordManager::markEncoderIdle()
{
    encoder_busy.store(false, std::memory_order_release);

    // Wake the sim worker (so it can proceed to produce the next frame)
    encoder_ready_cond.notify_all();
}



BL_END_NS;

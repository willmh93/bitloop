#include <bitloop/core/record_manager.h>
#include <bitloop/util/math_util.h>
#include <webp/encode.h>
#include <webp/mux.h>
#include <algorithm>

BL_BEGIN_NS;

static inline float srgb_to_linear_u8(uint8_t u) {
    // sRGB <-> linear (per channel, 0..255 <-> 0..1)
    float e = u / 255.0f;
    if (e <= 0.04045f) return e / 12.92f;
    return powf((e + 0.055f) / 1.055f, 2.4f);
}
static inline uint8_t linear_to_srgb_u8(float l) {
    l = std::clamp(l, 0.0f, 1.0f);
    double ld = static_cast<double>(l);
    double e  = (ld <= 0.0031308)
              ? (ld * 12.92)
              : (1.055 * std::pow(ld, 1.0 / 2.4) - 0.055);
    int v = static_cast<int>(e * 255.0 + 0.5);
    return static_cast<uint8_t>(std::clamp(v, 0, 255));
}
static void downscale_box_integer_rgba8_linear(const uint8_t* src, uint8_t* dst, int dst_w, int dst_h, int N)
{
    // Integer-factor box downscale in linear light (N×N average)
    int src_w = dst_w * N;
    //int src_h = dst_h * N;
    const int src_stride = src_w * 4;

    for (int dst_y = 0; dst_y < dst_h; ++dst_y)
    {
        const int src_y0 = dst_y * N;
        for (int dst_x = 0; dst_x < dst_w; ++dst_x)
        {
            const int src_x0 = dst_x * N;

            //const uint8_t* a = src + (src_y0 * src_w + src_x0) * 4;
            //uint8_t* b = dst + (dst_y * dst_w + dst_x) * 4;
            //b[0] = 255;//a[0];
            //b[1] = 255;// a[1];
            //b[2] = 255;// a[2];
            //b[3] = 255;//a[3];

            double accR = 0.0, accG = 0.0, accB = 0.0, accA = 0.0;
            for (int j = 0; j < N; ++j) {
                const uint8_t* row = src + (src_y0 + j) * src_stride + src_x0 * 4;
                for (int i = 0; i < N; ++i) {
                    const uint8_t* p = row + i * 4;
                    accR += srgb_to_linear_u8(p[0]);
                    accG += srgb_to_linear_u8(p[1]);
                    accB += srgb_to_linear_u8(p[2]);
                    accA += p[3] / 255.0;
                }
            }
            
            const double inv = 1.0 / double(N * N);
            const float r = float(accR * inv);
            const float g = float(accG * inv);
            const float b = float(accB * inv);
            const float a = float(accA * inv);
            
            uint8_t* q = dst + (dst_y * dst_w + dst_x) * 4;
            q[0] = linear_to_srgb_u8(r);
            q[1] = linear_to_srgb_u8(g);
            q[2] = linear_to_srgb_u8(b);
            q[3] = (uint8_t)std::lround(std::clamp(a, 0.0f, 1.0f) * 255.0f);
        }
    }
}
static inline float mitchell_kernel(float x) {
    // Mitchell–Netravali kernel (B=1/3, C=1/3), base radius=2
    const float B = 1.0f / 3.0f;
    const float C = 1.0f / 3.0f;
    x = fabsf(x);
    const float x2 = x * x;
    const float x3 = x2 * x;
    if (x < 1.0f) {
        return ((12 - 9 * B - 6 * C) * x3 + (-18 + 12 * B + 6 * C) * x2 + (6 - 2 * B)) / 6.0f;
    }
    else if (x < 2.0f) {
        return ((-B - 6 * C) * x3 + (6 * B + 30 * C) * x2 + (-12 * B - 48 * C) * x + (8 * B + 24 * C)) / 6.0f;
    }
    return 0.0f;
}
static void downscale_mitchell_rgba8_linear(const uint8_t* src, int sw, int sh, uint8_t* dst, int dw, int dh)
{
    // Separable resample (downscale) with Mitchell in linear light, supports fractional scale.
    // Note: for downscaling by S (>1), we widen the kernel by S (prefilter) to properly low-pass.
    const float scaleX = float(dw) / float(sw); // < 1 when downscaling
    const float scaleY = float(dh) / float(sh);
    const float sx = (scaleX < 1.0f) ? scaleX : 1.0f;
    const float sy = (scaleY < 1.0f) ? scaleY : 1.0f;

    // Horizontal pass to temp (linear floats)
    std::vector<float> tmp(dw * sh * 4, 0.0f);

    // Prepare horizontal
    const int radiusBase = 2; // Mitchell base radius
    for (int y = 0; y < sh; ++y) {
        for (int x = 0; x < dw; ++x) {
            // Map destination x to source space
            float srcX = (x + 0.5f) / scaleX - 0.5f;
            // Kernel width scales by 1/scale when downscaling
            float radius = radiusBase / sx;
            int x0 = int(floorf(srcX - radius));
            int x1 = int(ceilf(srcX + radius));

            double wsum = 0.0;
            double accR = 0.0, accG = 0.0, accB = 0.0, accA = 0.0;

            for (int ix = x0; ix <= x1; ++ix) {
                int sxclamp = std::min(std::max(ix, 0), sw - 1);
                float dx = (srcX - ix);
                // Scale distance by sx so kernel widens when downscaling
                float w = mitchell_kernel(dx * sx);
                if (w == 0.0f) continue;

                const uint8_t* p = src + (y * sw + sxclamp) * 4;
                // convert to linear
                float lr = srgb_to_linear_u8(p[0]);
                float lg = srgb_to_linear_u8(p[1]);
                float lb = srgb_to_linear_u8(p[2]);
                float la = p[3] / 255.0f;

                wsum += w;
                accR += w * lr;
                accG += w * lg;
                accB += w * lb;
                accA += w * la;
            }
            float r = (wsum > 0.0) ? float(accR / wsum) : 0.0f;
            float g = (wsum > 0.0) ? float(accG / wsum) : 0.0f;
            float b = (wsum > 0.0) ? float(accB / wsum) : 0.0f;
            float a = (wsum > 0.0) ? float(accA / wsum) : 0.0f;

            float* q = &tmp[(y * dw + x) * 4];
            q[0] = r; q[1] = g; q[2] = b; q[3] = a;
        }
    }

    // Vertical pass to dst (convert back to sRGB)
    for (int y = 0; y < dh; ++y) {
        float srcY = (y + 0.5f) / scaleY - 0.5f;
        float radius = radiusBase / sy;
        int y0 = int(floorf(srcY - radius));
        int y1 = int(ceilf(srcY + radius));

        for (int x = 0; x < dw; ++x) {
            double wsum = 0.0;
            double accR = 0.0, accG = 0.0, accB = 0.0, accA = 0.0;

            for (int iy = y0; iy <= y1; ++iy) {
                int syclamp = std::min(std::max(iy, 0), sh - 1);
                float dy = (srcY - iy);
                float w = mitchell_kernel(dy * sy);
                if (w == 0.0f) continue;

                const float* p = &tmp[(syclamp * dw + x) * 4];
                wsum += w;
                accR += w * p[0];
                accG += w * p[1];
                accB += w * p[2];
                accA += w * p[3];
            }
            float r = (wsum > 0.0) ? float(accR / wsum) : 0.0f;
            float g = (wsum > 0.0) ? float(accG / wsum) : 0.0f;
            float b = (wsum > 0.0) ? float(accB / wsum) : 0.0f;
            float a = (wsum > 0.0) ? float(accA / wsum) : 0.0f;

            uint8_t* q = dst + (y * dw + x) * 4;
            q[0] = linear_to_srgb_u8(r);
            q[1] = linear_to_srgb_u8(g);
            q[2] = linear_to_srgb_u8(b);
            q[3] = (uint8_t)std::round(fmaxf(0.0f, fminf(1.0f, a)) * 255.0f);
        }
    }
}
static void unsharp_linear_rgba8_inplace(uint8_t* img, int w, int h, float amount) {
    if (amount <= 0.0f) return;

    std::vector<float> lin(w * h * 3);
    // to linear
    for (int i = 0; i < w * h; ++i) {
        uint8_t* p = &img[i * 4];
        lin[i * 3 + 0] = srgb_to_linear_u8(p[0]);
        lin[i * 3 + 1] = srgb_to_linear_u8(p[1]);
        lin[i * 3 + 2] = srgb_to_linear_u8(p[2]);
    }
    // simple 3x3 box blur
    auto at = [&](int x, int y) { x = std::clamp(x, 0, w - 1); y = std::clamp(y, 0, h - 1); return (y * w + x) * 3; };
    std::vector<float> blur = lin;
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            double r = 0, g = 0, b = 0;
            for (int j = -1; j <= 1; ++j)
                for (int i = -1; i <= 1; ++i) {
                    int idx = at(x + i, y + j);
                    r += lin[idx + 0]; g += lin[idx + 1]; b += lin[idx + 2];
                }
            int o = (y * w + x) * 3;
            blur[o + 0] = float(r / 9.0); blur[o + 1] = float(g / 9.0); blur[o + 2] = float(b / 9.0);
        }
    }
    // sharpen and back to sRGB
    for (int i = 0; i < w * h; ++i) {
        float r = std::clamp(lin[i * 3 + 0] + amount * (lin[i * 3 + 0] - blur[i * 3 + 0]), 0.0f, 1.0f);
        float g = std::clamp(lin[i * 3 + 1] + amount * (lin[i * 3 + 1] - blur[i * 3 + 1]), 0.0f, 1.0f);
        float b = std::clamp(lin[i * 3 + 2] + amount * (lin[i * 3 + 2] - blur[i * 3 + 2]), 0.0f, 1.0f);
        uint8_t* p = &img[i * 4];
        p[0] = linear_to_srgb_u8(r);
        p[1] = linear_to_srgb_u8(g);
        p[2] = linear_to_srgb_u8(b);
    }
}

bool webp_snapshot_encode_rgba(
    const uint8_t* rgba,
    int w, int h,
    float quality,
    int method,
    bool lossless,
    int near_lossless,
    int alpha_quality,
    std::vector<uint8_t>& out)
{
    WebPConfig cfg;
    if (!WebPConfigInit(&cfg)) return false;

    if (lossless)
    {
        cfg.lossless = 1;
        WebPConfigLosslessPreset(&cfg, 9);
        cfg.quality = 100.0f;
        cfg.near_lossless = near_lossless;
        cfg.exact = 1;
        cfg.thread_level = 1;
        cfg.preprocessing = 0;
        cfg.alpha_quality = 0;
    }
    else {
        cfg.lossless = 0;
        cfg.quality = quality; // 0..100
        cfg.method = method;   // 0..6
        cfg.alpha_quality = alpha_quality;
        cfg.thread_level = 1;
    }

    if (!WebPValidateConfig(&cfg)) return false;

    WebPPicture pic;
    if (!WebPPictureInit(&pic)) return false;
    pic.width = w;
    pic.height = h;
    pic.use_argb = 1; // required for lossless

    if (!WebPPictureImportRGBA(&pic, rgba, w * 4)) {
        WebPPictureFree(&pic);
        return false;
    }

    WebPMemoryWriter wrt;
    WebPMemoryWriterInit(&wrt);
    pic.writer = WebPMemoryWrite;
    pic.custom_ptr = &wrt;

    const int ok = WebPEncode(&cfg, &pic);
    if (!ok) {
        WebPMemoryWriterClear(&wrt);
        WebPPictureFree(&pic);
        return false;
    }

    out.assign(wrt.mem, wrt.mem + wrt.size);
    WebPMemoryWriterClear(&wrt);
    WebPPictureFree(&pic);
    return true;
}

bool webp_anim_add_rgba(
    WebPAnimEncoder* enc,
    const uint8_t* rgba, int w, int h, int stride_bytes,
    int timestamp_ms,
    bool lossless,
    int near_lossless,
    float quality,
    int method)
{
    WebPConfig cfg;
    if (!WebPConfigInit(&cfg)) return false;

    if (lossless) {
        cfg.lossless = 1;
        WebPConfigLosslessPreset(&cfg, 9);
        cfg.quality = quality;   // effort in lossless
        cfg.near_lossless = near_lossless;      // strict lossless
        cfg.exact = 1;        // preserve RGB under alpha
        cfg.alpha_quality = 0;
        cfg.thread_level = 1;        // if this is stable in your WASM build
        cfg.preprocessing = 0;
        cfg.image_hint = WEBP_HINT_GRAPH;
    }
    else {
        cfg.lossless = 0;
        cfg.quality = quality;  // 0..100
        cfg.method = method;   // 0..6
        cfg.alpha_quality = 0; // or your own alpha q
        cfg.thread_level = 1;
    }

    if (!WebPValidateConfig(&cfg)) return false;

    WebPPicture pic;
    if (!WebPPictureInit(&pic)) return false;
    pic.width = w;
    pic.height = h;
    pic.use_argb = 1;

    if (!WebPPictureImportRGBA(&pic, rgba, stride_bytes)) {
        WebPPictureFree(&pic);
        return false;
    }

    const int ok = WebPAnimEncoderAdd(enc, &pic, timestamp_ms, &cfg);
    WebPPictureFree(&pic);
    return ok != 0;
}

bool webp_anim_encode_simple(
    const std::vector<bytebuf>& frames_rgba,
    int w, int h, int stride_bytes,
    int delays_ms,
    bool lossless, int near_lossless, float quality, int method,
    int loop_count,
    bytebuf& out_bytes)
{
    if (frames_rgba.empty()) return false;

    WebPAnimEncoderOptions opts;
    WebPAnimEncoderOptionsInit(&opts);
    opts.anim_params.bgcolor = 0x00000000; // RGBA
    opts.anim_params.loop_count = loop_count; // 0 = loop forever
    
    WebPAnimEncoder* enc = WebPAnimEncoderNew(w, h, &opts);
    if (!enc) return false;

    int t = 0;
    for (size_t i = 0; i < frames_rgba.size(); ++i) {
        if (!webp_anim_add_rgba(enc, frames_rgba[i].data(), w, h, stride_bytes, t,
            lossless, near_lossless, quality, method)) {
            WebPAnimEncoderDelete(enc);
            return false;
        }
        t += delays_ms;
    }

    // flush
    if (!WebPAnimEncoderAdd(enc, nullptr, t, nullptr)) {
        WebPAnimEncoderDelete(enc);
        return false;
    }

    WebPData webp = { 0 };
    const int ok = WebPAnimEncoderAssemble(enc, &webp);
    WebPAnimEncoderDelete(enc);
    if (!ok) return false;

    out_bytes.assign(webp.bytes, webp.bytes + webp.size);
    WebPDataClear(&webp);
    return true;
}

// ────── FFmpegWorker ──────

#ifndef __EMSCRIPTEN__
constexpr const char* codecs[] = { "x264", "x265" };
const char* VideoCodecFromCaptureFormat(CaptureFormat format)
{
    switch (format)
    {
    case CaptureFormat::x264: return codecs[0];
    case CaptureFormat::x265: return codecs[1];
    }
    return nullptr;
}

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

void FFmpegWorker::process(CaptureManager* capture_manager, CaptureConfig video_config)
{
    config = video_config;

    if (!startCapture())
        return;

    while (true)
    {
        if (!capture_manager->waitForWorkAvailable())
            break; // recording stopped, nothing left to do

        if (!capture_manager->isBusy())
        {
            // If finalize was requested and no frame is pending, flush and exit
            if (capture_manager->shouldFinalize() )
            {
                finalize(capture_manager);
                capture_manager->clearFinalizeRequest();
                break;
            }
        }
        else
        {
            // Grab the pending frame
            bytebuf frame = capture_manager->takePendingFrame();

            if (frame.empty())
            {
                // Shouldn't occur, but if it does, check we're still recording and skip frame
                if (!capture_manager->isRecording()) break;
                continue;
            }

            encodeFrame(frame);

            if (finalizing)
                finalize(capture_manager);

            // Tell capture_manager we're ready for the next frame
            capture_manager->markEncoderIdle();
        }

        if (capture_manager->shouldFinalize())
        {
            finalize(capture_manager);
            capture_manager->clearFinalizeRequest();
            break;
        }
    }
}

bool FFmpegWorker::startCapture()
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

    if (config.format == CaptureFormat::x265)
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

    //if (config.flip)
    //{
    //    int targ_y = 0;
    //    for (int src_y = trimmed_resolution.y - 1; src_y >= 0; src_y--)
    //    {
    //        // Copy row
    //        src_row = data + (src_y * src_line_size);
    //        targ_row = rgb_frame->data[0] + (targ_y * targ_line_size);
    //
    //        memcpy(targ_row, src_row, targ_line_size);
    //
    //        targ_y++;
    //    }
    //}
    //else
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



    return true;
}

bool FFmpegWorker::finalize(CaptureManager* capture_manager)
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

    capture_manager->onFinalized();

    return true;
}
#endif

// ────── WebPWorker ──────

void WebPWorker::process(CaptureManager* capture_manager, CaptureConfig video_config)
{
    config = video_config;

    if (!startCapture())
        return;

    blPrint() << "WebPWorker::process main loop...";
    while (true)
    {
        if (!capture_manager->waitForWorkAvailable())
            break; // recording stopped, nothing left to do

        blPrint() << "Found work...";

        if (!capture_manager->isBusy())
        {
            // If finalize was requested and no frame is pending, flush and exit
            if (capture_manager->shouldFinalize())
            {
                finalize(capture_manager);
                capture_manager->clearFinalizeRequest();
                break;
            }
        }
        else
        {
            blPrint() << "Grabbing pending frame...";

            // Grab the pending frame
            bytebuf frame = capture_manager->takePendingFrame();

            if (frame.empty())
            {
                blPrint() << "frame is empty";

                // Shouldn't occur, but if it does, check we're still recording and skip frame
                if (!capture_manager->isRecording() && !capture_manager->isSnapshotting()) break;
                continue;
            }

            encodeFrame(frame);

            // Tell capture_manager we're ready for the next frame
            capture_manager->markEncoderIdle();

            if (config.format == CaptureFormat::WEBP_SNAPSHOT)
            {
                finalize(capture_manager);
                break;
            }
            else if (config.format == CaptureFormat::WEBP_VIDEO &&
                     config.record_frame_count != 0 &&
                     frame_index >= config.record_frame_count)
            {
                finalize(capture_manager);
                break;
            }
        }

        if (capture_manager->shouldFinalize())
        {
            finalize(capture_manager);
            break;
        }
    }
    blPrint() << "Thread exiting...";
}

bool WebPWorker::startCapture()
{
    frame_index = 0;
    rgba_frames.clear();

    return true;
}

bool WebPWorker::encodeFrame(bytebuf& frame)
{
    blPrint() << "Encoding frame...";

    if (config.format == CaptureFormat::WEBP_SNAPSHOT)
    {
        webp_snapshot_encode_rgba(frame.data(), config.resolution.x, config.resolution.y, 100, 6, config.lossless, config.near_lossless, 100, encoded_data);
    }
    else if (config.format == CaptureFormat::WEBP_VIDEO)
    {
        rgba_frames.emplace_back();
        rgba_frames.back().swap(frame);
        frame_index++;
    }

    blPrint() << "Frame encoded";

    return true;
}

bool WebPWorker::finalize(CaptureManager* capture_manager)
{
    blPrint() << "<<Finalizing>>";

    if (config.format == CaptureFormat::WEBP_SNAPSHOT)
    {
        std::lock_guard<std::mutex> lock(capture_manager->encoded_data_mutex);
        capture_manager->encoded_data = std::move(encoded_data);
        capture_manager->capture_to_memory_complete.store(true, std::memory_order_release);
    }
    else if (config.format == CaptureFormat::WEBP_VIDEO)
    {
        webp_anim_encode_simple(
            rgba_frames,
            config.resolution.x, config.resolution.y,
            config.resolution.x * 4,
            (1000 / config.fps),
            config.lossless,
            config.near_lossless,
            config.quality,
            6, 0,
            encoded_data
        );

        std::lock_guard<std::mutex> lock(capture_manager->encoded_data_mutex);
        capture_manager->encoded_data = std::move(encoded_data);
        capture_manager->capture_to_memory_complete.store(true, std::memory_order_release);
    }

    capture_manager->onFinalized();

    return true;
}

// ────── CaptureManager ──────

bool CaptureManager::startCapture(
    CaptureFormat format,
    std::string filename,
    IVec2 resolution,
    int supersample_factor,
    float sharpen,
    int fps,
    int record_frame_count,
    int64_t bitrate,
    float quality,
    bool lossless,
    int near_lossless,
    bool flip)
{
    config.format = format;
    config.filename = filename;
    config.resolution = resolution;
    config.sharpen = sharpen;
    config.ssaa = supersample_factor;
    config.flip = flip;
    config.record_frame_count = record_frame_count;
    config.fps = fps;
    config.bitrate = bitrate;
    config.quality = quality;
    config.lossless = lossless;
    config.near_lossless = near_lossless;

    capture_to_memory_complete.store(false, std::memory_order_release);

    if (format == CaptureFormat::x264 || format == CaptureFormat::x265)
    {
        #ifndef __EMSCRIPTEN__
        recording.store(true, std::memory_order_release);

        encoder_thread = std::jthread(&FFmpegWorker::process, &ffmpeg_worker, this, config);
        #else
        blPrint() << "Error: FFmpeg not supported on web";
        #endif
    }
    else if (format == CaptureFormat::WEBP_VIDEO)
    {
        recording.store(true, std::memory_order_release);

        encoder_thread = std::jthread(&WebPWorker::process, &webp_worker, this, config);
    }
    else if (format == CaptureFormat::WEBP_SNAPSHOT)
    {
        snapshotting.store(true, std::memory_order_release);

        encoder_thread = std::jthread(&WebPWorker::process, &webp_worker, this, config);
    }

    blPrint() << "startRecording()";

    return true;
}

void CaptureManager::finalizeCapture()
{
    // public facing:
    //   ask encoder thread to finalize after the next frame (or immediately if idle)
        
    finalize_requested.store(true, std::memory_order_release);
    work_available_cond.notify_one();
}

void CaptureManager::onFinalized()
{
    // called by encoder thread upon finalized...

    blPrint() << "onFinalized()";

    // No longer recording/snapshotting
    recording.store(false, std::memory_order_release);
    snapshotting.store(false, std::memory_order_release);

    // Encoder no longer busy
    encoder_busy.store(false, std::memory_order_release);

    clearFinalizeRequest();

    // No more work to do - notify waiters.
    /// todo: Still needed if we only ever call from encoder thread?
    work_available_cond.notify_all();

    // Wake any sim thread
    encoder_ready_cond.notify_all();

    blPrint() << "finalizeCapture()";
}

void CaptureManager::preProcessFrameForEncoding(const uint8_t* src_data, bytebuf& out)
{
    int src_w = config.resolution.x * config.ssaa;
    int src_h = config.resolution.y * config.ssaa;
    int dest_w = config.resolution.x;
    int dest_h = config.resolution.y;

    // flip src_data => oriented_src_data
    if (config.flip)
    {
        oriented_src_data.resize(config.srcBytes());

        const uint8_t* src_row = nullptr;

        uint8_t* dst_data = oriented_src_data.data();
        uint8_t* dst_row = nullptr;

        int line_size = src_w * 4;
        for (int src_y = src_h - 1, dst_y = 0; src_y >= 0; src_y--, dst_y++)
        {
            src_row = src_data + (src_y * line_size);
            dst_row = dst_data + (dst_y * line_size);
            memcpy(dst_row, src_row, line_size);
        }
    }
    else
    {
        oriented_src_data = bytebuf(src_data, src_data + config.srcBytes());
    }

    // Downscale "oriented_src_data" to "out"
    {
        out.resize(config.dstBytes());

        const uint8_t* src_ptr = oriented_src_data.data();
        uint8_t* dst_ptr = out.data();

        const float ssaa_f = (float)config.ssaa;
        const int   ssaa_i = (int)std::round(ssaa_f);
        const bool  is_integer = (fabsf(ssaa_f - (float)ssaa_i) < 1e-6f);

        if (is_integer && ssaa_i > 1)
            // integer scaling
            downscale_box_integer_rgba8_linear(src_ptr, dst_ptr, dest_w, dest_h, ssaa_i);
        else if (ssaa_f > 1.0f)
            // Fractional scaling
            downscale_mitchell_rgba8_linear(src_ptr, src_w, src_h, dst_ptr, dest_w, dest_h);
        else
            // No scaling: just copy
            std::memcpy(dst_ptr, src_ptr, out.size());

        // apply sharpening
        unsharp_linear_rgba8_inplace(dst_ptr, dest_w, dest_h, config.sharpen);
    }
}

bool CaptureManager::encodeFrame(const uint8_t* data)
{
    if (!isCaptureEnabled())
        return false;

    // Preprocess frame (supersampling, sharpening, flipping, etc...)
    /// todo: Move to worker?
    blPrint() << "preProcessFrameForEncoding()...";
    bytebuf preprocessed_frame;
    preProcessFrameForEncoding(data, preprocessed_frame);

    // If encoder still busy with previous frame, do not block here
    if (encoder_busy.load(std::memory_order_acquire))
        return false;

    // Move the frame into 'pending_frame' for the encoder thread to grab
    {
        blPrint() << "Set pending_frame...";

        std::lock_guard<std::mutex> lock(pending_mutex);
        pending_frame.resize(config.srcBytes());
        std::memcpy(pending_frame.data(), preprocessed_frame.data(), config.srcBytes());

        // Mark busy only after the buffer is fully populated
        blPrint() << "encoder_busy.store(true)";
        encoder_busy.store(true, std::memory_order_release);
    }

    work_available_cond.notify_one();

    ///if (isSnapshotting())
    ///{
    ///    // Auto-finalize snapshots on the first frame
    ///    finalizeCapture();
    ///}

    return true;

    /*#ifndef __EMSCRIPTEN__
    if (isRecording())
    {
        // Wake the FFmpeg worker, a new frame is ready to encode

        return true;
    }
    else
    #endif
    if (isSnapshotting())
    {
        bytebuf encoded;
        webp_snapshot_encode_rgba(preprocessed_frame.data(), config.resolution.x, config.resolution.y, 100, 6, true, 100, encoded);
        finalizeSnapshot(encoded);

        return true;
    }

    return false;*/
}

void CaptureManager::waitUntilReadyForNewFrame()
{
    blPrint() << "waitUntilReadyForNewFrame()...";
    if (!isCaptureEnabled())
    {
        blPrint() << "isCaptureEnabled() == false";

        return;
    }

    blPrint() << "encoder_ready_mutex lock";
    std::unique_lock<std::mutex> lock(encoder_ready_mutex);


    blPrint() << "encoder_ready_cond.wait";
    encoder_ready_cond.wait(lock, [this]
    {
        return !encoder_busy.load(std::memory_order_acquire) || !(isRecording() || isSnapshotting());
    });
}

bool CaptureManager::waitForWorkAvailable()
{
    blPrint() << "Waiting for work...";
    std::unique_lock<std::mutex> lock(work_available_mutex);
    work_available_cond.wait(lock, [this]{
        return encoder_busy.load(std::memory_order_acquire) || finalize_requested.load(std::memory_order_acquire);
    });

    return true;
}

bytebuf CaptureManager::takePendingFrame()
{
    blPrint() << "takePendingFrame()...";
    std::lock_guard<std::mutex> lock(pending_mutex);
    bytebuf out;
    out.swap(pending_frame);
    return out;
}

void CaptureManager::markEncoderIdle()
{
    blPrint() << "markEncoderIdle()";
    encoder_busy.store(false, std::memory_order_release);

    // Wake the sim worker (so it can proceed to produce the next frame)
    encoder_ready_cond.notify_all();
}

BL_END_NS;

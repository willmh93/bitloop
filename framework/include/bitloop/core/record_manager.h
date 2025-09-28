#pragma once
#include <bitloop/core/debug.h>
#include <bitloop/core/types.h>

#include <string>
#include <mutex>
#include <thread>
#include <atomic>
#include <vector>

#ifndef __EMSCRIPTEN__
extern "C" {
    #include <libavcodec/avcodec.h>
    #include <libavformat/avformat.h>
    #include <libswscale/swscale.h>
    #include <libavutil/avutil.h>
    #include <libavutil/opt.h>
}
#endif

BL_BEGIN_NS;

typedef std::vector<uint8_t> bytebuf;

struct VideoConfig
{
    std::string filename;
    IVec2 resolution;
    int  fps = 0;
    bool flip = true;

    size_t numBytes() const { return (size_t)resolution.x * (size_t)resolution.y * 4; }
};

class RecordManager;
class FFmpegWorker
{
    friend class RecordManager;

    AVFormatContext* format_context  = nullptr;
    AVStream*        stream          = nullptr;
    AVCodecContext*  codec_context   = nullptr;
    AVFrame*         yuv_frame       = nullptr;
    AVFrame*         rgb_frame       = nullptr;
    SwsContext*      sws_ctx         = nullptr;
    AVPacket*        packet          = nullptr;

    VideoConfig config;
    IVec2       trimmed_resolution; // rounded down to nearest even number

    // encoding states
    int  frame_index = 0;
    bool finalizing = false;

    // main thread worker
    void process(RecordManager* record_manager, VideoConfig config);

    bool startRecording();
    bool encodeFrame(bytebuf& frame);
    bool finalizeRecording();
};


class RecordManager
{
    friend class FFmpegWorker;

    FFmpegWorker ffmpeg_worker;
    std::thread  ffmpeg_worker_thread;

    std::atomic<bool> recording{ false };
    std::atomic<bool> encoder_busy{ false };
    std::atomic<bool> finalize_requested{ false };

    // sim worker waits on this to know encoder is ready for a new frame
    std::condition_variable encoder_ready_cond;
    std::mutex              encoder_ready_mutex;

    // worker waits on this to know a frame is available
    std::condition_variable work_available_cond;
    std::mutex              work_available_mutex;

    // single pending frame
    std::mutex pending_mutex;
    bytebuf    pending_frame;

    VideoConfig config;

    // ────── methods used by the worker thread ──────

    bool    waitForWorkAvailable(); // waits until a frame is pending (or returns false if not recording / no work)
    bytebuf takePendingFrame();     // then swaps out the pending frame (takes ownership)
    void    markEncoderIdle();      // then marks as idle when finished encoding

    void requestFinalizeAfterNextFrame(); // ask the ffmpeg worker to finalize after the next frame (or immediately if idle)
    bool shouldFinalize() const { return finalize_requested.load(std::memory_order_acquire); }
    void clearFinalizeRequest() { finalize_requested.store(false, std::memory_order_release); }

public:

    // record_manager should exist for entire MainWindow lifecycle, but finalize recording in case of forceful exit.
    ~RecordManager() { finalizeRecording(); }

    IVec2 getResolution() const { return config.resolution; }
    int fps() const { return config.fps; }

    bool isRecording() const    { return recording.load(std::memory_order_acquire); }
    bool isBusy() const         { return encoder_busy.load(std::memory_order_acquire); }

    bool startRecording(
        std::string filename,
        IVec2 resolution,
        int fps, bool flip);

    void waitUntilReadyForNewFrame(); // sim worker calls this avoid avoid drawing a frame until ffmpeg worker is ready for it
    bool encodeFrame(const uint8_t* data);
    void finalizeRecording();
};

BL_END_NS;

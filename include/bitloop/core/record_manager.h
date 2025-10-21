#pragma once
#include <bitloop/core/debug.h>
#include <bitloop/core/types.h>

#include <string>
#include <mutex>
#include <thread>
#include <atomic>
#include <vector>

#if BITLOOP_FFMPEG_ENABLED
extern "C" {
    #include <libavcodec/avcodec.h>
    #include <libavformat/avformat.h>
    #include <libswscale/swscale.h>
    #include <libavutil/avutil.h>
    #include <libavutil/opt.h>
}
#endif

/* =========================== CaptureManager ===========================
*  
*  - Responsible for capturing both images (snapshotting) and videos (recording).
*
*  - It's considered to be "capturing" if recording OR snapshotting.
*
*  - A snapshot is not guaranteed to be captured immediately in the case
*    of it being incrementally generated over several frames at the requested
*    resolution (e.g. Mandelbrot).
*
*    It waits for a simulation to call captureFrame(true) and is treated
*    like a single-frame video but fed to an Image encoder (e.g. AVIF/WEBP_SNAPSHOT) 
*    instead of the FFmpeg video encoder.
*/

BL_BEGIN_NS;

enum struct CaptureFormat
{
    // Video
    x264,
    x265,
    WEBP_VIDEO,

    // Snapshot
    WEBP_SNAPSHOT
};

struct BitrateRange
{
    double min_mbps;
    double max_mbps;
};

struct CaptureConfig
{
    CaptureFormat   format;

    // Generic
    std::string     filename;
    IVec2           resolution;
    int             ssaa; // supersample_factor
    float           sharpen; // 0 - 1

    float           quality = 100.0;

    // WebP generic
    bool            lossless = true;
    int             near_lossless = 100;

    // Video generic
    int             fps = 0;
    int             record_frame_count = 0;
    bool            flip = true;

    // ffmpeg
    int64_t         bitrate = 0;
    bool            ten_bit = true;

    size_t srcBytes() const { return (size_t)(resolution.x * ssaa) * (size_t)(resolution.y * ssaa) * 4; }
    size_t dstBytes() const { return (size_t)resolution.x * (size_t)resolution.y * 4; }
};

class CaptureManager;

#if BITLOOP_FFMPEG_ENABLED
const char* VideoCodecFromCaptureFormat(CaptureFormat format);

class FFmpegWorker
{
    friend class CaptureManager;

    AVFormatContext* format_context  = nullptr;
    AVStream*        stream          = nullptr;
    AVCodecContext*  codec_context   = nullptr;
    AVFrame*         yuv_frame       = nullptr;
    AVFrame*         rgb_frame       = nullptr;
    SwsContext*      sws_ctx         = nullptr;
    AVPacket*        packet          = nullptr;
    
    CaptureConfig    config;
    IVec2            trimmed_resolution; // rounded down to nearest even number

    int              frame_index = 0;
    bool             finalizing = false;

    // main thread worker
    void process(CaptureManager* capture_manager, CaptureConfig config);

    bool startCapture();
    bool encodeFrame(bytebuf& frame);
    bool finalize(CaptureManager* capture_manager);
};
#endif

class WebPWorker
{
    friend class CaptureManager;

    CaptureConfig   config;

    std::vector<bytebuf> rgba_frames;
    bytebuf encoded_data;

    int frame_index = 0;

    // main thread worker
    void process(CaptureManager* capture_manager, CaptureConfig config);

    bool startCapture();
    bool encodeFrame(bytebuf& frame);
    bool finalize(CaptureManager* capture_manager);
};


class CaptureManager
{
    CaptureConfig config;

    // Can be assigned to WebPWorker or FFmpegWorker
    std::jthread   encoder_thread;

    friend class  WebPWorker;
    WebPWorker    webp_worker;

    #if BITLOOP_FFMPEG_ENABLED
    friend class  FFmpegWorker;
    FFmpegWorker  ffmpeg_worker;
    #endif

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
    bytebuf    pending_frame; // grabbed by encoder thread with std::move

    std::atomic<bool> capture_enabled{ true };

    std::atomic<bool> recording{ false };
    std::atomic<bool> snapshotting{ false };

    // intermediate buffer, flipped if (config.flipped == true), otherwise unchanged
    bytebuf  oriented_src_data;

    // WebPWorker stores final encoded data in encoded_data and sets capture_to_memory_complete=true
    std::atomic<bool> capture_to_memory_complete{ false };
    std::mutex encoded_data_mutex;
    bytebuf    encoded_data;

    // ────── methods used by the worker thread ──────

    bool     waitForWorkAvailable(); // waits until a frame is pending (or returns false if not recording / no work)
    bytebuf  takePendingFrame();     // then swaps out the pending frame (takes ownership)
    void     markEncoderIdle();      // then marks as idle when finished encoding

    bool     shouldFinalize() const  { return finalize_requested.load(std::memory_order_acquire); }
    void     clearFinalizeRequest()  { finalize_requested.store(false, std::memory_order_release); }
    void     onFinalized();

    void     preProcessFrameForEncoding(const uint8_t* src_data, bytebuf& out);

public:

    // CaptureManager should exist for entire MainWindow lifecycle, but finalize recording in case of forceful exit.
    ~CaptureManager()                { finalizeCapture(); }
                                     
    IVec2        srcResolution() const  { return config.resolution * config.ssaa; }
    IVec2        dstResolution() const  { return config.resolution; }
    int          fps() const            { return config.fps;        }
    std::string  filename() const       { return config.filename;   }
                                     
    bool  isRecording() const           { return recording.load(std::memory_order_acquire);        }
    bool  isSnapshotting() const        { return snapshotting.load(std::memory_order_acquire);     }
    bool  isCaptureEnabled() const      { return capture_enabled.load(std::memory_order_acquire);  }
    void  setCaptureEnabled(bool b)     { capture_enabled.store(b, std::memory_order_release);     }
                                        
    bool  isBusy() const                { return encoder_busy.load(std::memory_order_acquire);     }

    // NOT called for encoders which write media directly to filesystem like ffmpeg, 
    // only for encoders which write directly to memory (which we handle manually on the main thread)
    bool  isCaptureToMemoryComplete() const     { return capture_to_memory_complete.load(std::memory_order_acquire); }
    void  takeCompletedCaptureFromMemory(bytebuf &out)
    {
        capture_to_memory_complete.store(false, std::memory_order_release);
        out = std::move(encoded_data);
    }

    

    // sim worker calls this avoid triggering a draw a frame until ffmpeg worker is ready for it
    void  waitUntilReadyForNewFrame();

    // ────── [start capture] => [encode frames] => [end capture] ──────

    bool  startCapture(
        CaptureFormat format,
        std::string file,
        IVec2 resolution,
        int supersample_factor=1,
        float sharpen=0.0f,
        int fps=0,
        int record_frame_count = 0,
        int64_t bitrate=0,
        float quality=100.0,
        bool lossless=true,
        int near_lossless=100,
        bool flip=false);

    bool  startCapture(
        CaptureFormat format,
        IVec2 resolution,
        int supersample_factor = 1,
        float sharpen = 0.0f,
        int fps = 0,
        int record_frame_count=0,
        int64_t bitrate = 0,
        float quality = 100.0,
        bool lossless = true,
        int near_lossless = 100,
        bool flip = false)
    {
        return startCapture(
            format, "", resolution, supersample_factor,
            sharpen, fps, record_frame_count, bitrate,
            quality, lossless, near_lossless, flip);
    }

    bool  encodeFrame(const uint8_t* data);

    void  finalizeCapture();

};

BL_END_NS;

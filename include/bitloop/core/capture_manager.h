#pragma once
#include <bitloop/core/debug.h>
#include <bitloop/core/types.h>
#include <bitloop/core/capture_preprocessor.h>
#include <bitloop/util/hashable.h>

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
*    It waits for a simulation to call captureFrame(true) when ready, and is treated
*    like a single-frame video but fed to an Image encoder (CaptureFormat::WEBP_SNAPSHOT) 
*    instead of the WEBP/FFmpeg multi-frame encoder.
*/

class WebPAnimEncoder;

BL_BEGIN_NS;

enum struct CaptureFormat
{
    // Video
    #if BITLOOP_FFMPEG_ENABLED
        x264,
        #if BITLOOP_FFMPEG_X265_ENABLED
            x265,
        #endif
    #endif

    WEBP_VIDEO,
    WEBP_SNAPSHOT
};

inline const char* CaptureFormatComboString()
{
    static const char* ret =
        #if BITLOOP_FFMPEG_ENABLED
            "H.264 (x264)\0"
            #if BITLOOP_FFMPEG_X265_ENABLED
                "H.265 / HEVC (x265)\0"
            #endif
        #endif
        "WebP Animation\0"
        "WebP Snapshot\0";

    return ret;
}

struct BitrateRange
{
    double min_mbps;
    double max_mbps;
};

// CaptureManager input config
struct CaptureConfig
{
    CaptureFormat   format;

    // Generic
    std::string     filename;
    IVec2           resolution;
    int             ssaa = 1;    // supersample_factor
    float           sharpen = 0; // 0 - 1

    float           quality = 100.0;

    // WebP generic
    bool            lossless = true;
    int             near_lossless = 100;
    std::string     save_payload; // optional metadata

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

struct EncodeFrame
{
    bytebuf data;
    std::string payload; // XMP data (or other metadata)

    EncodeFrame() = default;
    EncodeFrame(const bytebuf& d) : data(d) {}
    EncodeFrame(const bytebuf& d, std::string_view p) : data(d), payload(p) {}

    size_t size() const { return data.size(); }
    void resize(size_t size) { data.resize(size); }

    operator bytebuf& () { return data; }
    uint8_t* frameData() { return data.data(); }
    const uint8_t* frameData() const { return data.data(); }
    bool empty() const { return data.empty(); }

    void loadFrom(const EncodeFrame& src)
    {
        resize(src.size());
        std::memcpy(frameData(), src.frameData(), src.size());
        payload = src.payload;
    }

    void swap(EncodeFrame& other)
    {
        data.swap(other.data);
        payload.swap(other.payload);
    }

    uint8_t& operator[](size_t i) { return data[i]; }
};

class CaptureManager;

#if BITLOOP_FFMPEG_ENABLED
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
    void process(CaptureManager* capture_manager, CaptureConfig capture_config);

    bool startCapture();
    bool encodeFrame(EncodeFrame& frame);
    bool finalize(CaptureManager* capture_manager);
};
#endif

bool webp_set_save_string_as_xmp_inplace(bytebuf& io_webp, std::string_view save_text);
bool webp_extract_save_from_xmp(const bytebuf& webp, std::string& out_save);

class WebPWorker
{
    friend class CaptureManager;

    CaptureConfig   config;

    WebPAnimEncoder* enc = nullptr;
    bytebuf encoded_data;

    int frame_index = 0;
    int timestamp_ms = 0;
    int frame_delay_ms = 0;

    // main thread worker
    void process(CaptureManager* capture_manager, CaptureConfig config);

    bool startCapture();
    bool encodeFrame(EncodeFrame& frame);
    bool finalize(CaptureManager* capture_manager);
};


class CaptureManager
{
    CaptureConfig config;

    CapturePreprocessor preprocessor;

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
    EncodeFrame pending_frame; // grabbed by encoder thread with std::move

    std::atomic<bool> capture_enabled{ true };

    std::atomic<bool> recording{ false };
    std::atomic<bool> snapshotting{ false };

    // WebPWorker stores final encoded data in encoded_data and sets capture_to_memory_complete=true
    std::atomic<bool> capture_to_memory_complete{ false };
    std::atomic<bool> any_capture_complete{ false };
    std::mutex encoded_data_mutex;
    bytebuf    encoded_data;

    // ────── methods used by the worker thread ──────

    bool         waitForWorkAvailable(); // waits until a frame is pending (or returns false if not recording / no work)
    EncodeFrame  takePendingFrame();     // then swaps out the pending frame (takes ownership)
    void         markEncoderIdle();      // then marks as idle when finished encoding

    bool     shouldFinalize() const  { return finalize_requested.load(std::memory_order_acquire); }
    void     clearFinalizeRequest()  { finalize_requested.store(false, std::memory_order_release); }
    void     onFinalized();

    void     preProcessFrameForEncoding(const uint8_t* src_data, bytebuf& out);

public:

    // CaptureManager should exist for entire MainWindow lifecycle, but finalize recording in case of forceful exit.
    ~CaptureManager()                { finalizeCapture(); }
                                     
    IVec2          srcResolution() const  { return config.resolution * config.ssaa; }
    IVec2          dstResolution() const  { return config.resolution; }
    int            fps() const            { return config.fps;        }
    std::string    filename() const       { return config.filename;   }
    CaptureFormat  format() const         { return config.format;     }
                                     
    bool  isRecording() const           { return recording.load(std::memory_order_acquire);        }
    bool  isSnapshotting() const        { return snapshotting.load(std::memory_order_acquire);     }
    bool  isCapturing() const           { return isRecording() || isSnapshotting();                }
    bool  isCaptureEnabled() const      { return capture_enabled.load(std::memory_order_acquire);  }
    void  setCaptureEnabled(bool b)     { capture_enabled.store(b, std::memory_order_release);     }
                                        
    bool  isBusy() const                { return encoder_busy.load(std::memory_order_acquire);     }

    bool  handleCaptureComplete(bool& captured_to_memory)
    {
        bool complete = any_capture_complete.load(std::memory_order_acquire);
        if (complete)
        {
            captured_to_memory = capture_to_memory_complete.load(std::memory_order_acquire);

            // reset flags
            capture_to_memory_complete.store(false, std::memory_order_release);
            any_capture_complete.store(false, std::memory_order_release);
        }
        
        return complete;
    }

    void  takeCompletedCaptureFromMemory(bytebuf &out)
    {
        //capture_to_memory_complete.store(false, std::memory_order_release);
        out = std::move(encoded_data);
    }

    // sim worker calls this avoid triggering a draw a frame until ffmpeg worker is ready for it
    void  waitUntilReadyForNewFrame();

    // ────── [start capture] => [encode frames] => [finalize capture] ──────

    bool startCapture(CaptureConfig _config);
    bool encodeFrame(const uint8_t* data,
        //std::function<void(EncodeFrame&)> preProcessedFrame = nullptr,
        std::function<void(EncodeFrame&)> postProcessedFrame = nullptr);

    bool encodeFrameTexture(uint32_t src_texture,
        //std::function<void(EncodeFrame&)> preProcessedFrame = nullptr,
        std::function<void(EncodeFrame&)> postProcessedFrame = nullptr);

    void finalizeCapture();

};

BL_END_NS;

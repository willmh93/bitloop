#pragma once
#include <bitloop/core/debug.h>
#include <bitloop/core/types.h>

#include <cstdint>

BL_BEGIN_NS;

struct CapturePreprocessParams
{
    IVec2   src_resolution{};
    IVec2   dst_resolution{};

    int     ssaa = 1;
    float   sharpen = 0.0f; // 0..1
    bool    flip_y = true;
};

class CapturePreprocessor
{
public:
    CapturePreprocessor() = default;
    CapturePreprocessor(const CapturePreprocessor&) = delete;
    CapturePreprocessor& operator=(const CapturePreprocessor&) = delete;

    ~CapturePreprocessor();

    // source is a GL_TEXTURE_2D containing RGBA8 in sRGB space.
    bool preprocessTexture(uint32_t src_texture, const CapturePreprocessParams& params, bytebuf& out_rgba);

    // gpu-only preprocess into internal output texture
    bool preprocessTextureToTexture(uint32_t src_texture, const CapturePreprocessParams& params);

    // preprocess into an externally owned framebuffer (expects color attachment 0)
    bool preprocessTextureToFbo(uint32_t src_texture, const CapturePreprocessParams& params, uint32_t dst_fbo);

    // preprocess into an externally owned framebuffer and read back RGBA8
    bool preprocessTextureToFbo(uint32_t src_texture, const CapturePreprocessParams& params, uint32_t dst_fbo, bytebuf& out_rgba);

    // upload + preprocess from CPU RGBA8
    bool preprocessRGBA8(const uint8_t* src_rgba, const CapturePreprocessParams& params, bytebuf& out_rgba);

    // internal output GL_TEXTURE_2D. Valid after any successful internal-output preprocess call
    uint32_t outputTexture() const { return output_tex; }

    // internal output resolution
    IVec2 outputResolution() const { return target_size; }

    // explicit teardown while a GL context is current
    void destroyGL();

private:
    bool ensureInitialized();
    bool ensureDownTarget(const IVec2& dst_resolution);
    bool ensureOutputTarget(const IVec2& dst_resolution);
    bool ensureUploadTexture(const IVec2& src_resolution);

    bool runPipeline(uint32_t src_texture, const CapturePreprocessParams& params, bytebuf* out_rgba, uint32_t dst_fbo_override);

    bool compilePrograms();
    void updateShaderVersionStrings();

private:
    bool initialized = false;
    bool is_gles = false;

    uint32_t vao = 0;

    uint32_t upload_tex = 0;
    IVec2    upload_size{};

    uint32_t down_tex = 0;
    IVec2    down_size{};

    uint32_t output_tex = 0;
    IVec2    target_size{};

    uint32_t fbo_down = 0;
    uint32_t fbo_out = 0;

    uint32_t prog_down = 0;
    uint32_t prog_unsharp = 0;

    // cached uniform locations
    int loc_down_src = -1;
    int loc_down_srcSize = -1;
    int loc_down_ssaa = -1;
    int loc_down_flipY = -1;

    int loc_unsharp_tex = -1;
    int loc_unsharp_size = -1;
    int loc_unsharp_amount = -1;
};

BL_END_NS;

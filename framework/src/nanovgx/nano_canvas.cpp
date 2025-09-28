#ifdef __EMSCRIPTEN__
#define NANOVG_GLES3_IMPLEMENTATION
#else
#define NANOVG_GL3_IMPLEMENTATION
#endif

#include <bitloop/nanovgx/nano_canvas.h>
#include <bitloop/core/project.h>

BL_BEGIN_NS

constexpr double nice_steps[] = { 1.0, 2.0, 5.0, 10.0 };
constexpr int    nice_steps_count = 4;

inline flt128 niceStepDivisible(flt128 ideal, flt128& coarse, flt128& fade)
{
    if (ideal <= flt128{ 0.0 })
    {
        coarse = fade = flt128{ 0.0 };
        return flt128{ 0.0 };
    }

    const flt128 exp10 = floor(log10(ideal));

    // Collect nice numbers for current decade and one above
    constexpr int span_count = 2; // exp10 and exp10+1 are enough to contain ideal
    std::array<flt128, nice_steps_count * span_count> candidates{};
    int n = 0;
    for (int d = 0; d <= 1; ++d) 
    {
        const flt128 v = pow(flt128{ 10.0 }, exp10 + flt128{ (double)d });
        for (int i = 0; i < nice_steps_count; ++i)
            candidates[n++] = nice_steps[i] * v;
    }
    std::sort(candidates.begin(), candidates.begin() + n);

    // Fine step: largest candidate <= ideal
    flt128 step = candidates[0];
    for (int i = 0; i < n && candidates[i] <= ideal; ++i) 
        step = candidates[i];

    // Coarse step: first candidate > step that is divisible by step
    coarse = step * 2.0; // fallback
    for (int i = 0; i < n; ++i) 
    {
        if (candidates[i] > step && Math::divisible(candidates[i], step)) 
        { 
            coarse = candidates[i];
            break; 
        }
    }

    // Calculate fade (how far ideal lies between step and coarse)
    fade = clamp((ideal - step) / (coarse - step), flt128{ 0.0 }, flt128{ 1.0 });
    return step;
}

void Painter::drawWorldAxis(
    double axis_opacity,
    double grid_opacity,
    double text_opacity)
{
    save();
    camera.saveCameraTransform();

    Viewport* ctx = camera.viewport;
    const double angle = camera.rotation();
    const DDVec2 zoom = camera.zoom<flt128>();

    // ======== viewport world bounds ========
    const DDVec2 TL = camera.toWorld<flt128>(0, 0);
    const DDVec2 TR = camera.toWorld<flt128>(ctx->width(), 0);
    const DDVec2 BR = camera.toWorld<flt128>(ctx->width(), ctx->height());
    const DDVec2 BL = camera.toWorld<flt128>(0, ctx->height());

    // ======== min/max of world bounds ========
    const flt128 wMinX = std::min({ TL.x, TR.x, BR.x, BL.x });
    const flt128 wMaxX = std::max({ TL.x, TR.x, BR.x, BL.x });
    const flt128 wMinY = std::min({ TL.y, TR.y, BR.y, BL.y });
    const flt128 wMaxY = std::max({ TL.y, TR.y, BR.y, BL.y });

    // ======== Step calculation ========
    const flt128 targetPx = flt128{ scale_size(140.0) };
    flt128 coarseX, coarseY, fadeX, fadeY;
    const flt128 stepX = niceStepDivisible(targetPx / zoom.x, coarseX, fadeX);
    const flt128 stepY = niceStepDivisible(targetPx / zoom.y, coarseY, fadeY);

    camera.worldHudTransform();

    // ======== grid lines ========
    if (grid_opacity > 0.0) 
    {
        setLineWidth(1);
        constexpr flt128 kMinorFactor = flt128{ 0.25 }; // opacity for fine lines

        auto gridPass = [&](bool isX)
        {
            const flt128 step   = isX ? stepX : stepY;
            const flt128 coarse = isX ? coarseX : coarseY;
            const flt128 a0     = isX ? wMinX : wMinY;
            const flt128 a1     = isX ? wMaxX : wMaxY;

            // Loop over each gridline
            for (flt128 w = Math::roundUp(a0, step); w < a1; w += step)
            {
                const bool major = abs((w / coarse) - round(w / coarse)) < flt128::eps();
                const flt128 alpha = major ? flt128{ 1.0 } : kMinorFactor;

                setStrokeStyle(255, 255, 255, static_cast<int>(grid_opacity * alpha * 255));
                if (isX) strokeLineSharp<flt128>({w, wMinY}, {w, wMaxY});
                else     strokeLineSharp<flt128>({wMinX, w}, {wMaxX, w});
            }
        };

        gridPass(true);   // vertical lines
        gridPass(false);  // horizontal lines
    }

    // ======== World axes ========
    if (axis_opacity > 0.0) 
    {
        setStrokeStyle(255, 255, 255, static_cast<int>(axis_opacity * 255.0));
        setLineWidth(1);
        strokeLineSharp<flt128>({wMinX, f128(0)}, {wMaxX, f128(0)});
        strokeLineSharp<flt128>({f128(0), wMinY}, {f128(0), wMaxY});
    }

    // ======== Tick marks and labels ========
    if (axis_opacity > 0.0 || text_opacity > 0.0) 
    {
        const double tick_length = scale_size(6);

        // Align all text by center
        camera.stageTransform();
        setTextAlign(TextAlign::ALIGN_CENTER);
        setTextBaseline(TextBaseline::BASELINE_MIDDLE);
        setStrokeStyle(255, 255, 255, static_cast<int>(axis_opacity * 255.0));

        // Get typical size and use same height for all numbers (regular & scientific)
        DVec2 standard_txt_size = boundingBox("W").size() / 2.0;
        auto tickPass = [&](bool isX) 
        {
            const flt128 step = isX ? stepX : stepY;
            const flt128 coarse = isX ? coarseX : coarseY;
            const flt128 fade = isX ? fadeX : fadeY;
            DVec2 perpDir = camera.axisStagePerpDirection(isX);
            const flt128 wStart = isX ? wMinX : wMinY;
            const flt128 wEnd = isX ? wMaxX : wMaxY;
            double txt_sample_angle = isX ? angle : (angle + Math::HALF_PI);
            double txt_size_weight_x = std::abs(std::cos(txt_sample_angle));
            double txt_size_weight_y = std::abs(std::sin(txt_sample_angle));

            for (flt128 w = Math::roundUp(wStart, step); w < wEnd; w += step)
            {
                if (abs(w) < flt128::eps()) continue;
                const bool major = abs((w / coarse) - round(w / coarse)) < flt128::eps();
                const flt128 alphaL = major ? flt128{ 1.0 } : (flt128{ 1.0 } - fade);
                const int aTick = static_cast<int>(axis_opacity * alphaL * 255.0);
                const int txt_alpha = static_cast<int>(text_opacity * alphaL * 255.0);

                const DVec2 stage_pos = isX ? camera.toStage<flt128>(w, f128(0)) : camera.toStage<flt128>(f128(0), w);
                setStrokeStyle(255, 255, 255, aTick);
                strokeLineSharp(DVec2{stage_pos - perpDir * tick_length}, DVec2{stage_pos + perpDir * tick_length});

                if (txt_alpha > 0)
                {
                    constexpr double spacing = 3;
                    DVec2 txt_size = DVec2{ boundingBoxScientific(w).size().x * 0.75, standard_txt_size.y };
                    double txt_dist = (txt_size_weight_x * txt_size.y) + (txt_size_weight_y * txt_size.x) * 0.5 + spacing;

                    DVec2 txt_anchor = stage_pos + (perpDir * (tick_length + txt_dist));
                    setFillStyle(255, 255, 255, txt_alpha);
                    fillNumberScientific(w, txt_anchor);
                }
            }
        };

        tickPass(true);  // x axis
        tickPass(false); // y axis
    }
    
    restore();
    camera.restoreCameraTransform();
}

void Canvas::create(double _global_scale)
{
    #ifdef __EMSCRIPTEN__
    context.vg = nvgCreateGLES3(NVG_ANTIALIAS | NVG_STENCIL_STROKES);
    #else
    context.vg = nvgCreateGL3(NVG_ANTIALIAS | NVG_STENCIL_STROKES);
    #endif
    usePainter(&context);
    
    setGlobalScale(_global_scale);


    //SimplePainter::default_font = NanoFont::create("/data/fonts/Roboto-Regular.ttf");
    context.default_font = NanoFont::create("/data/fonts/UbuntuMono.ttf");
    context.default_font->setSize(16.0f);
}

bool Canvas::resize(int w, int h)
{
    if ((w == fbo_width && fbo_height == h) ||
        (w <= 0 || h <= 0))
    {
        return false;
    }

    fbo_width = w;
    fbo_height = h;

    if (fbo) glDeleteFramebuffers(1, &fbo);
    if (tex) glDeleteTextures(1, &tex);
    if (rbo) glDeleteRenderbuffers(1, &rbo);

    glGenFramebuffers(1, &fbo);
    glGenTextures(1, &tex);
    glGenRenderbuffers(1, &rbo);

    glBindTexture(GL_TEXTURE_2D, tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

    glBindRenderbuffer(GL_RENDERBUFFER, rbo);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, w, h);

    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex, 0);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    has_fbo = true;

    return true;
}

void Canvas::begin(float r, float g, float b, float a)
{
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glViewport(0, 0, fbo_width, fbo_height);
    glClearColor(r, g, b, a);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    nvgBeginFrame(context.vg, 
        static_cast<float>(fbo_width), 
        static_cast<float>(fbo_height),
        static_cast<float>(context.global_scale) // Improve render quality on high DPR devices
    );
}

void Canvas::end()
{
    nvgEndFrame(context.vg);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

bool Canvas::readPixels(std::vector<uint8_t>& out_rgba)
{
    if (!fbo || fbo_width <= 0 || fbo_height <= 0) return false;
    const size_t bytes = (size_t)fbo_width * fbo_height * 4;
    out_rgba.resize(bytes);

    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);

    glReadPixels(0, 0, fbo_width, fbo_height, GL_RGBA, GL_UNSIGNED_BYTE, out_rgba.data());
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    return true;
}

BL_END_NS

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

inline double niceStepDivisible(double ideal, double& coarse, double& fade)
{
    if (ideal <= 0) 
    {
        coarse = fade = 0; 
        return 0; 
    }

    const double exp10 = std::floor(std::log10(ideal));

    // Collect nice numbers for current decade and one above
    constexpr int span_count = 2; // exp10 and exp10+1 are enough to contain ideal
    std::array<double, nice_steps_count * span_count> candidates{};
    int n = 0;
    for (int d = 0; d <= 1; ++d) 
    {
        const double v = std::pow(10.0, exp10 + d);
        for (int i = 0; i < nice_steps_count; ++i)
            candidates[n++] = nice_steps[i] * v;
    }
    std::sort(candidates.begin(), candidates.begin() + n);

    // Fine step: largest candidate <= ideal
    double step = candidates[0];
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
    fade = std::clamp((ideal - step) / (coarse - step), 0.0, 1.0);
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
    double angle = camera.rotation();

    // ======== viewport world bounds ========
    const DVec2 TL = camera.toWorld(0, 0);
    const DVec2 TR = camera.toWorld(ctx->width(), 0);
    const DVec2 BR = camera.toWorld(ctx->width(), ctx->height());
    const DVec2 BL = camera.toWorld(0, ctx->height());

    // ======== min/max of world bounds ========
    const double wMinX = std::min({ TL.x, TR.x, BR.x, BL.x });
    const double wMaxX = std::max({ TL.x, TR.x, BR.x, BL.x });
    const double wMinY = std::min({ TL.y, TR.y, BR.y, BL.y });
    const double wMaxY = std::max({ TL.y, TR.y, BR.y, BL.y });

    // ======== Step calculation ========
    const double targetPx = scale_size(140.0);
    double coarseX, coarseY, fadeX, fadeY;
    const double stepX = niceStepDivisible(targetPx / camera.zoomX(), coarseX, fadeX);
    const double stepY = niceStepDivisible(targetPx / camera.zoomY(), coarseY, fadeY);

    camera.worldHudTransform();

    // ======== grid lines ========
    if (grid_opacity > 0.0) 
    {
        setLineWidth(1);
        constexpr double kMinorFactor = 0.25;// 0.25; // opacity for fine lines

        auto gridPass = [&](bool isX)
        {
            const double step   = isX ? stepX : stepY;
            const double coarse = isX ? coarseX : coarseY;
            const double a0     = isX ? wMinX : wMinY;
            const double a1     = isX ? wMaxX : wMaxY;

            // Loop over each gridline
            for (double w = Math::roundUp(a0, step); w < a1; w += step) 
            {
                const bool major = std::abs((w / coarse) - std::round(w / coarse)) < 1e-9;
                const double alpha = major ? 1.0 : kMinorFactor;

                setStrokeStyle(255, 255, 255, static_cast<int>(grid_opacity * alpha * 255));
                if (isX) strokeLineSharp({w, wMinY}, {w, wMaxY});
                else     strokeLineSharp({wMinX, w}, {wMaxX, w});
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
        strokeLineSharp({wMinX, 0}, {wMaxX, 0});
        strokeLineSharp({0, wMinY}, {0, wMaxY});
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
            const double step = isX ? stepX : stepY;
            const double coarse = isX ? coarseX : coarseY;
            const double fade = isX ? fadeX : fadeY;
            DVec2 perpDir = camera.axisStagePerpDirection(isX);
            const double wStart = isX ? wMinX : wMinY;
            const double wEnd = isX ? wMaxX : wMaxY;
            double txt_sample_angle = isX ? angle : (angle + Math::HALF_PI);
            double txt_size_weight_x = std::abs(std::cos(txt_sample_angle));
            double txt_size_weight_y = std::abs(std::sin(txt_sample_angle));

            for (double w = Math::roundUp(wStart, step); w < wEnd; w += step)
            {
                if (std::abs(w) < 1e-12) continue;
                const bool major = std::abs((w / coarse) - std::round(w / coarse)) < 1e-9;
                const double alphaL = (major ? 1.0 : (1.0 - fade));
                const int aTick = static_cast<int>(axis_opacity * alphaL * 255.0);
                const int txt_alpha = static_cast<int>(text_opacity * alphaL * 255.0);

                const DVec2 stage_pos = isX ? camera.toStage(w, 0) : camera.toStage(0, w);
                setStrokeStyle(255, 255, 255, aTick);
                strokeLineSharp({stage_pos - perpDir * tick_length}, {stage_pos + perpDir * tick_length});

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
    setPainterContext(&context);
    
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

BL_END_NS

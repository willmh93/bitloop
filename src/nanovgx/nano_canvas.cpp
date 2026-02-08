#include <bitloop/nanovgx/nano_canvas.h>
#include <bitloop/core/project.h>

BL_BEGIN_NS

constexpr double nice_steps[] = { 1.0, 2.0, 5.0, 10.0 };
constexpr int    nice_steps_count = 4;

template<typename flt>
inline flt niceStepDivisible(flt ideal, flt& coarse, flt& fade)
{
    if (ideal <= 0.0)
    {
        coarse = fade = 0;
        return 0;
    }

    const int exp10 = (int)floor(log10(ideal));

    // Collect nice numbers for current decade and one above
    constexpr int span_count = 2; // exp10 and exp10+1 are enough to contain ideal
    std::array<flt, nice_steps_count * span_count> candidates{};
    int n = 0;
    for (int d = 0; d <= 1; ++d) 
    {
        //const f128 v = pow(10, exp10 + f128(d));
        const flt v = (flt)pow10_128(exp10 + d); // todo: optimize for f64 (don't use f128 pow10)
        for (int i = 0; i < nice_steps_count; ++i)
            candidates[n++] = nice_steps[i] * v;
    }
    std::sort(candidates.begin(), candidates.begin() + n);

    // Fine step: largest candidate <= ideal
    flt step = candidates[0];
    for (int i = 0; i < n && candidates[i] <= ideal; ++i) 
        step = candidates[i];

    // Coarse step: first candidate > step that is divisible by step
    coarse = step * 2.0; // fallback
    for (int i = 0; i < n; ++i) 
    {
        if (candidates[i] > step && math::divisible(candidates[i], step)) 
        { 
            coarse = candidates[i];
            break;
        }
    }

    // Calculate fade (how far ideal lies between step and coarse)
    fade = bl::clamp((ideal - step) / (coarse - step), 0.0, 1.0);
    return step;
}

void Painter::drawWorldAxis(
    double axis_opacity,
    double grid_opacity,
    double text_opacity)
{
    save();
    saveCameraTransform();

    typedef f64 flt;
    typedef Vec2<flt> vec2 ;

    //Viewport* ctx = camera.viewport;
    const double angle = m._angle();
    //const DDVec2 zoom(camera.zoom<f128>(), camera.zoom<f128>());
    const vec2 zoom(m._zoom<flt>());

    // ======== viewport world bounds ========
    const vec2 TL = m.toWorld<flt>(0, 0);
    const vec2 TR = m.toWorld<flt>(surface->width(), 0);
    const vec2 BR = m.toWorld<flt>(surface->width(), surface->height());
    const vec2 BL = m.toWorld<flt>(0, surface->height());

    // ======== min/max of world bounds ========
    const flt wMinX = std::min({ TL.x, TR.x, BR.x, BL.x });
    const flt wMaxX = std::max({ TL.x, TR.x, BR.x, BL.x });
    const flt wMinY = std::min({ TL.y, TR.y, BR.y, BL.y });
    const flt wMaxY = std::max({ TL.y, TR.y, BR.y, BL.y });

    // ======== Step calculation ========
    const flt targetPx = flt{ scale_size(140.0) };
    flt coarseX, coarseY, fadeX, fadeY;
    const flt stepX = niceStepDivisible<flt>(targetPx / zoom.x, coarseX, fadeX);
    const flt stepY = niceStepDivisible<flt>(targetPx / zoom.y, coarseY, fadeY);

    worldHudMode();

    // ======== grid lines ========
    if (grid_opacity > 0.0) 
    {
        setLineWidth(1);
        constexpr flt kMinorFactor = flt{ 0.25 }; // opacity for fine lines

        auto gridPass = [&](bool isX)
        {
            const flt step   = isX ? stepX : stepY;
            const flt coarse = isX ? coarseX : coarseY;
            const flt a0     = isX ? wMinX : wMinY;
            const flt a1     = isX ? wMaxX : wMaxY;
            const flt eps = std::numeric_limits<flt>::epsilon();

            // Loop over each gridline
            for (flt w = math::roundUp(a0, step); w < a1; w += step)
            {
                const bool major = abs((w / coarse) - bl::round(w / coarse)) < eps;
                const flt alpha = major ? 1 : kMinorFactor;

                setStrokeStyle(255, 255, 255, static_cast<int>(grid_opacity * alpha * 255));
                if (isX) strokeLineSharp<flt>({w, wMinY}, {w, wMaxY});
                else     strokeLineSharp<flt>({wMinX, w}, {wMaxX, w});
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
        strokeLineSharp<flt>({wMinX, 0}, {wMaxX, 0});
        strokeLineSharp<flt>({0, wMinY}, {0, wMaxY});
    }

    // ======== Tick marks and labels ========
    if (axis_opacity > 0.0 || text_opacity > 0.0) 
    {
        const double tick_length = scale_size(6);

        // Align all text by center
        stageMode();
        setTextAlign(TextAlign::ALIGN_CENTER);
        setTextBaseline(TextBaseline::BASELINE_MIDDLE);
        setStrokeStyle(255, 255, 255, static_cast<int>(axis_opacity * 255.0));

        // Get typical size and use same height for all numbers (regular & scientific)
        DVec2 standard_txt_size = boundingBox("W").size() / 2.0;
        auto tickPass = [&](bool isX) 
        {
            const flt step = isX ? stepX : stepY;
            const flt coarse = isX ? coarseX : coarseY;
            const flt fade = isX ? fadeX : fadeY;
            DVec2 perpDir = m.axisStagePerpDirection(isX);
            const flt wStart = isX ? wMinX : wMinY;
            const flt wEnd = isX ? wMaxX : wMaxY;
            double txt_sample_angle = isX ? angle : (angle + math::half_pi);
            double txt_size_weight_x = std::abs(std::cos(txt_sample_angle));
            double txt_size_weight_y = std::abs(std::sin(txt_sample_angle));

            int decimals = 0;
            if (isX)
            {
                if (zoom.x > 1.0)
                    decimals = (1 + math::countWholeDigits(zoom.x * 0.0125));
            }
            else
            {
                if (zoom.y > 1.0)
                    decimals = (1 + math::countWholeDigits(zoom.y * 0.0125));
            }

            // multiply epsilon by large number of potential steps to avoid rounding error
            const flt eps = 100.0 * std::numeric_limits<flt>::epsilon();

            for (flt w = math::roundUp(wStart, step); w < wEnd; w += step)
            {
                if (abs(w) < eps) continue;
                const bool major = abs((w / coarse) - round(w / coarse)) < eps;
                const flt alphaL = major ? 1 : (1 - fade);
                const int aTick = static_cast<int>(axis_opacity * alphaL * 255.0);
                const int txt_alpha = static_cast<int>(text_opacity * alphaL * 255.0);

                const DVec2 stage_pos = isX ? m.toStage<flt>(w, 0) : m.toStage<flt>(0, w);
                setStrokeStyle(255, 255, 255, aTick);
                strokeLineSharp(DVec2{stage_pos - perpDir * tick_length}, DVec2{stage_pos + perpDir * tick_length});

                std::string txt = format_number(w, decimals);

                constexpr double spacing = 3;
                DVec2 txt_size = DVec2{ boundingBoxScientific<f64, flt>(txt).size().x * 0.75, standard_txt_size.y };
                double txt_dist = (txt_size_weight_x * txt_size.y) + (txt_size_weight_y * txt_size.x) * 0.5 + spacing;

                DVec2 txt_anchor = stage_pos + (perpDir * (tick_length + txt_dist));
                setFillStyle(255, 255, 255, txt_alpha);
                fillNumberScientific<f64, flt>(txt, txt_anchor);
            }
        };

        tickPass(true);  // x axis
        tickPass(false); // y axis
    }
    
    restore();
    restoreCameraTransform();
}

void Canvas::create(double _global_scale)
{
    context.vg = nvgCreate(NVG_ANTIALIAS | NVG_STENCIL_STROKES);
    usePainter(&context);
    
    setGlobalScale(_global_scale);

    context.default_font = NanoFont::create("/data/fonts/UbuntuMono.ttf");
}

Canvas::~Canvas()
{
    if (fbo) glDeleteFramebuffers(1, &fbo);
    if (tex) glDeleteTextures(1, &tex);
    if (rbo) glDeleteRenderbuffers(1, &rbo);
    if (context.vg)
    {
        nvgDelete(context.vg);
        context.vg = nullptr;
    }
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
    GLint prev_pack = 4;
    glGetIntegerv(GL_PACK_ALIGNMENT, &prev_pack);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);

    glReadPixels(0, 0, fbo_width, fbo_height, GL_RGBA, GL_UNSIGNED_BYTE, out_rgba.data());
    glPixelStorei(GL_PACK_ALIGNMENT, prev_pack);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    return true;
}

BL_END_NS

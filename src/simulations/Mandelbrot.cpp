#include "Mandelbrot.h"

//SIM_SORT_KEY(0)
SIM_DECLARE(Mandelbrot, "Fractal", "Mandelbrot", "Viewer")


///---------///
/// Project ///
///---------///

void Mandelbrot_Project::projectAttributes()
{

}

void Mandelbrot_Project::projectPrepare()
{
    auto& layout = newLayout();

    Mandelbrot_Scene::Config config1;
    //auto config2 = make_shared<Test_Scene::Config>(Test_Scene::Config());

    create<Mandelbrot_Scene>(config1)->mountTo(layout);
}

///-----------///
/// SceneBase ///
///-----------///

void Mandelbrot_Scene::sceneAttributes()
{
    x_spline.set(x_spline_points, (int)ImSpline::PointsArrSize(x_spline_points), 1000);
    y_spline.set(y_spline_points, (int)ImSpline::PointsArrSize(y_spline_points), 1000);

    static ImRect vr = { 0.0f, 0.8f, 0.8f, 0.0f };
    ImSpline::SplineEditorPair("X/Y Spline", &x_spline, &y_spline, &vr, 900.0f);

    /// --------------------------------------------------------------
    ImGui::SeparatorText("Compute");
    /// --------------------------------------------------------------

    ImGui::Checkbox("Discrete Step", &discrete_step);
    if (discrete_step)
        ImGui::DragDouble("Steps", &quality, 1000.0, 1.0, 1000000.0, "%.0f", ImGuiSliderFlags_Logarithmic);
    else
        ImGui::SliderDouble("Quality", &quality, 1.0, 150.0);


    /// --------------------------------------------------------------
    ImGui::SeparatorText("Main Cardioid");
    /// --------------------------------------------------------------

    ImGui::Checkbox("Flatten", &flatten_main_cardioid);
    if (flatten_main_cardioid)
    {
        if (ImGui::SliderDouble("Flatness", &flatten_amount, 0.0, 1.0))
            cardioid_lerp_amount = 1.0 - flatten_amount;
    }

    ImGui::Checkbox("Show period-2 bulb", &show_period2_bulb);
    ImGui::Checkbox("Interactive", &interactive_cardioid);


    /// --------------------------------------------------------------
    ImGui::SeparatorText("Visual Options");
    /// --------------------------------------------------------------

    ImGui::Checkbox("Thresholding", &thresholding);
    if (!thresholding)
        ImGui::Checkbox("Smoothing", &smoothing);


    /// --------------------------------------------------------------
    ImGui::SeparatorText("View");
    /// --------------------------------------------------------------

    ImGui::SliderDouble("Rotation", &cam_rotation, 0.0, M_PI * 2.0);

    // 1e16 = double limit before preicions loss
    ImGui::DragDouble("Zoom Mult", &cam_zoom, cam_zoom / 100, 300.0, 1e16);
    ImGui::SliderDouble2("Zoom X/Y", cam_zoom_xy, 0.1, 10.0);

    // CPU only
    ///if (ImGui::InputTextMultiline("Config", config_buf, 1024))
    ///{
    ///    loadConfigBuffer();
    ///}
}

void Mandelbrot_Scene::sceneStart()
{
    //bmp.create(700, 700);

    cardioid_lerper.create((2.0 * M_PI) / 5760, 0.005);
}

void Mandelbrot_Scene::sceneMounted(Viewport* viewport)
{
    camera->setOriginViewportAnchor(Anchor::CENTER);
    camera->setPanningUsesOffset(false);
    camera->focusWorldRect(-2, -1, 2, 1);
    cam_zoom = camera->zoom_x;
}

void Mandelbrot_Scene::step_color(double step, uint8_t& r, uint8_t& g, uint8_t& b)
{
    double ratio;
    if (smoothing)
        ratio = fmod(step, 1.0);
    else
        ratio = fmod(step / 10.0, 1.0);

    r = (uint8_t)(9 * (1 - ratio) * ratio * ratio * ratio * 255);
    g = (uint8_t)(15 * (1 - ratio) * (1 - ratio) * ratio * ratio * 255);
    b = (uint8_t)(8.5 * (1 - ratio) * (1 - ratio) * (1 - ratio) * ratio * 255);
}

void Mandelbrot_Scene::iter_ratio_color(double ratio, uint8_t& r, uint8_t& g, uint8_t& b)
{
    if (thresholding)
    {
        r = g = b = (ratio <= 0.9999) ? 0 : 127;
        return;
    }
    r = (uint8_t)(9 * (1 - ratio) * ratio * ratio * ratio * 255);
    g = (uint8_t)(15 * (1 - ratio) * (1 - ratio) * ratio * ratio * 255);
    b = (uint8_t)(8.5 * (1 - ratio) * (1 - ratio) * (1 - ratio) * ratio * 255);
}

void Mandelbrot_Scene::cpu_mandelbrot(
    Viewport* ctx,
    double fw, double fh,
    double wx0, double wy0,
    double ww, double wh,
    int pixel_count)
{
    int iw = static_cast<int>(fw);
    int ih = static_cast<int>(fw);
    double f_max_iter = static_cast<double>(iter_lim);


    if (!flatten_main_cardioid)
    {
        // Standard
        bool linear = false;// x_spline.isSimpleLinear() && y_spline.isSimpleLinear();

        dispatchBooleans(
            boolsTemplate(regularMandelbrot, [&], ctx),
            smoothing,
            linear,
            show_period2_bulb
        );
    }
    else
    {
        // Flat lerp
        dispatchBooleans(
            boolsTemplate(radialMandelbrot, [&], ctx),
            smoothing,
            show_period2_bulb
        );
    }
}

void Mandelbrot_Scene::sceneDestroy()
{
}

void Mandelbrot_Scene::sceneProcess()
{
}

void Mandelbrot_Scene::viewportProcess(Viewport* ctx)
{
    /// Process Viewports running this Scene
    double fw = ctx->width;
    double fh = ctx->height;
    int iw = static_cast<int>(fw);
    int ih = static_cast<int>(fh);
    int pixel_count = iw * ih;

    FRect stage_rect_on_world = ctx->camera.toWorldRect(0, 0, ctx->width, ctx->height);
    double wx1 = stage_rect_on_world.x1;
    double wx2 = stage_rect_on_world.x2;
    double wy1 = stage_rect_on_world.y1;
    double wy2 = stage_rect_on_world.y2;
    double ww = wx2 - wx1;
    double wh = wy2 - wy1;

    if (discrete_step)
        iter_lim = static_cast<int>(quality);
    else
        iter_lim = static_cast<int>(std::log2(camera->zoom_x) * quality);

    double f_max_iter = static_cast<double>(iter_lim);

    if (anyChanged(
        quality,
        smoothing,
        thresholding,
        discrete_step,
        flatten_main_cardioid,
        show_period2_bulb,
        cardioid_lerp_amount,
        x_spline.hash(),
        y_spline.hash() )) 
    {
        bmp.setNeedsReshading();
    }

    camera->rotation = cam_rotation;
    camera->setZoomX(cam_zoom * cam_zoom_xy[0]);
    camera->setZoomY(cam_zoom * cam_zoom_xy[1]);

    bmp.setStageRect(0, 0, ctx->width, ctx->height);
    bmp.setBitmapSize(iw, ih);

    if (bmp.needsReshading(camera))
    {
        //updateConfigBuffer();
        cpu_mandelbrot(ctx, fw, fh, wx1, wy1, ww, wh, pixel_count);
    }
}

void Mandelbrot_Scene::viewportDraw(Viewport* ctx)
{
    camera->stageTransform();
    ctx->drawImage(bmp);
    ctx->drawWorldAxis(0.5, 0, 0.5);

    if (interactive_cardioid)
    {
        if (!flatten_main_cardioid)
        {
            // Regular interactive Mandelbrot
            //if (!log_x && !log_y)
            //    Cardioid::plot(this, ctx, true);
        }
        else
        {
            // Lerped/Flattened Mandelbrot
            camera->scalingLines(false);
            ctx->setLineWidth(1);
            ctx->beginPath();
            ctx->drawPath(cardioid_lerper.lerped(cardioid_lerp_amount));
            ctx->stroke();
        }
    }

    ///ctx->print() << "Frame delta time: " << this->project_dt(10) << " ms";
    ///ctx->print() << "\nMax Iterations: " << iter_lim;
    /////ctx->print() << "\nLinear: " << x_spline.isLinear();
    ///
    ///double zoom_factor = camera->getRelativeZoomFactor().average();
    ///ctx->print() << "\nZoom: " << QString::asprintf("%.1f", zoom_factor) << "x";
}

/// User Interaction

/*void Test_Scene::mouseDown()
{
    //ball_pos.x = mouse.world_x;
    //ball_pos.y = mouse.world_y;
}
void Test_Scene::mouseUp()
{
    //ball_pos.x = mouse.world_x;
    //ball_pos.y = mouse.world_y;
}
void Test_Scene::mouseMove()
{
    ball_pos.x = mouse->world_x;
    ball_pos.y = mouse->world_y;
}
void Test_Scene::mouseWheel() {}*/

SIM_END(Mandelbrot)

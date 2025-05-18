#include "project.h"

/// Event


// Touch helpers

double Event::finger_x()
{
    return (double)(_event.tfinger.x * (float)Platform()->drawable_width());
}

double Event::finger_y()
{
    return (double)(_event.tfinger.y * (float)Platform()->drawable_height());
}

std::string Event::info()
{
    std::string focused = _focused_ctx ? std::to_string(_focused_ctx->viewportIndex()) : "_";
    std::string hovered = _hovered_ctx ? std::to_string(_hovered_ctx->viewportIndex()) : "_";
    std::string type;

    char attribs[256];
    
    switch (_event.type) 
    {
    case SDL_KEYDOWN:           type = "SDL_KEYDOWN";         break;
    case SDL_KEYUP:             type = "SDL_KEYUP";           break;

    case SDL_MOUSEMOTION:       type = "SDL_MOUSEMOTION";     
        sprintf(attribs, "{%d, %d}",
            _event.motion.x,
            _event.motion.y); 

        break;

    case SDL_MOUSEBUTTONDOWN:   type = "SDL_MOUSEBUTTONDOWN";  goto mouse_attribs;
    case SDL_MOUSEBUTTONUP:     type = "SDL_MOUSEBUTTONUP";    goto mouse_attribs;
        mouse_attribs:
        sprintf(attribs, "{%d, %d}", 
            _event.button.x, 
            _event.button.y); 

        break;

    case SDL_FINGERDOWN:        type = "SDL_FINGERDOWN";    goto finger_attribs;
    case SDL_FINGERUP:          type = "SDL_FINGERUP";      goto finger_attribs;
    case SDL_FINGERMOTION:      type = "SDL_FINGERMOTION";
        finger_attribs:
        sprintf(attribs, "{F_%lld, %d, %d}", 
            _event.tfinger.fingerId, 
            (int)(_event.tfinger.x * (float)Platform()->drawable_width()),
            (int)(_event.tfinger.y * (float)Platform()->drawable_height()));

        break;

    case SDL_MULTIGESTURE:      type = "SDL_MULTIGESTURE";     break;

    case SDL_MOUSEWHEEL:        type = "SDL_MOUSEWHEEL";       break;
    case SDL_TEXTINPUT:         type = "SDL_TEXTINPUT";        break;

    default:                    type = "UNKNOWN_EVENT";        break;
    }

    std::string info;
    info += "[focused: " + focused + "] ";
    info += "[hovered: " + hovered + "] ";
    info += "[" + type + "] ";
    info += attribs;

    return info;
}

/// Scene

void SceneBase::registerMount(Viewport* viewport)
{
    //options = viewport->options;
    //camera = &viewport->camera;

    mounted_to_viewports.push_back(viewport);
    ///qDebug() << "Mounted to Viewport: " << viewport->viewportIndex();

    std::vector<SceneBase*>& all_scenes = viewport->layout->all_scenes;

    // Only add scene to list if it's not already in the layout (mounted to another viewport)
    if (std::find(all_scenes.begin(), all_scenes.end(), this) == all_scenes.end())
        viewport->layout->all_scenes.push_back(this);
}

void SceneBase::registerUnmount(Viewport* viewport)
{
    std::vector<SceneBase*>& all_scenes = viewport->layout->all_scenes;

    // Only remove scene from list if the layout includes it (which it should)
    auto scene_it = std::find(all_scenes.begin(), all_scenes.end(), this);
    if (scene_it != all_scenes.end())
    {
        ///qDebug() << "Erasing project scene from viewport: " << viewport->viewportIndex();
        all_scenes.erase(scene_it);
    }

    // Only unmount from viewport if the layout includes the viewport (which it should)
    auto viewport_it = std::find(mounted_to_viewports.begin(), mounted_to_viewports.end(), viewport);
    if (viewport_it != mounted_to_viewports.end())
    {
        ///qDebug() << "Unmounting scene from viewport: " << viewport->viewportIndex();
        mounted_to_viewports.erase(viewport_it);
    }
}

void SceneBase::mountTo(Viewport* viewport)
{
    viewport->mountScene(this);
}

void SceneBase::mountTo(Layout& viewports)
{
    viewports[viewports.count()]->mountScene(this);
}

void SceneBase::mountToAll(Layout& viewports)
{
    for (Viewport* viewport : viewports)
        viewport->mountScene(this);
}

double SceneBase::frame_dt(int average_samples) const
{
    if (dt_call_index >= dt_scene_ma_list.size())
        dt_scene_ma_list.push_back(Math::MovingAverage::MA(average_samples));

    auto& ma = dt_scene_ma_list[dt_call_index++];
    return ma.push(project->dt_frameProcess);
}

double SceneBase::scene_dt(int average_samples) const
{
    if (dt_call_index >= dt_scene_ma_list.size())
        dt_scene_ma_list.push_back(Math::MovingAverage::MA(average_samples));

    auto& ma = dt_scene_ma_list[dt_call_index++];
    return ma.push(dt_sceneProcess);
}

double SceneBase::project_dt(int average_samples) const
{
    if (dt_process_call_index >= dt_project_ma_list.size())
        dt_project_ma_list.push_back(Math::MovingAverage::MA(average_samples));

    auto& ma = dt_project_ma_list[dt_process_call_index++];
    return ma.push(project->dt_projectProcess);
}

///  Viewport

Viewport::Viewport(Layout* layout, Canvas* canvas, int viewport_index, int grid_x, int grid_y) :
    layout(layout),
    viewport_index(viewport_index),
    viewport_grid_x(grid_x),
    viewport_grid_y(grid_y)
{
    setRenderTarget(canvas->getRenderTarget());
    camera.viewport = this;
    ///print_stream.setString(&print_text);
}

Viewport::~Viewport()
{
    if (scene)
    {
        // Unmount sim from viewport
        scene->registerUnmount(this);

        // If sim is no longer mounted to any viewports, it's safe to destroy
        if (scene->mounted_to_viewports.size() == 0)
        {
            ///qDebug() << "Unmounted scene is mounted to no other viewports. Destroying scene";
            scene->sceneDestroy();
            delete scene;
        }
    }
    ///qDebug() << "Viewport destroyed: " << viewport_index;
}

void Viewport::draw()
{
    // Set defaults
    setFont(getDefaultFont());
    setTextAlign(TextAlign::ALIGN_LEFT);
    setTextBaseline(TextBaseline::BASELINE_TOP);

    // Snapshot default transformation (unscaled unrotated top-left viewport)
    default_viewport_transform = currentTransform();

    // When resizing window, world coordinate is fixed given viewport anchor
    // If TOP_LEFT, the world coordinate at top left remains fixed
    // If CENTER, world coordinate at middle of viewport remains fixed

    translate(
        floor(camera.originPixelOffset().x + camera.panPixelOffset().x),
        floor(camera.originPixelOffset().y + camera.panPixelOffset().y)
    );

    rotate(camera.rotation);
    translate(
        floor(-camera.x * camera.zoom_x),
        floor(-camera.y * camera.zoom_y)
    );

    scale(camera.zoom_x, camera.zoom_y);

    /// GL Transform/Projection
    /*{
        QMatrix4x4 _projectionMatrix;
        QMatrix4x4 _transformMatrix;
        _projectionMatrix.ortho(0, width, height, 0, -1, 1);  // Top-left origin
        _transformMatrix.setToIdentity();

        // Do transformations
        _transformMatrix.translate(
            floor(camera.originPixelOffset().x + camera.panPixelOffset().x),
            floor(camera.originPixelOffset().y + camera.panPixelOffset().y)
        );
        _transformMatrix.rotate(camera.rotation, 0.0f, 0.0f, 1.0f);
        _transformMatrix.translate(
            floor(-camera.x * camera.zoom_x),
            floor(-camera.y * camera.zoom_y)
        );
        _transformMatrix.scale(camera.zoom_x, camera.zoom_y);

        projectionMatrix = _projectionMatrix;
        transformMatrix = _transformMatrix;
    }

    print_text = "";*/

    print_stream.str("");
    print_stream.clear();

    // Draw mounted project to this viewport
    scene->camera = &camera;
    scene->viewportDraw(this);

    // Draw print() lines at top-left of viewport
    save();
    camera.saveCameraTransform();
    camera.stageTransform();
    setTextAlign(TextAlign::ALIGN_LEFT);
    setTextBaseline(TextBaseline::BASELINE_TOP);
    setFillStyle(255, 255, 255);

    print_stream.seekg(0);
    int line_index = 0;
    std::string line;
    while (std::getline(print_stream, line, '\n'))
        fillText(line, 5, 5 + (line_index++ * 16));

    camera.restoreCameraTransform();
    restore();
}

double roundAxisTickStep(double ideal_step)
{
    if (ideal_step <= 0)
        return 0; // or handle error

    // Determine the order of magnitude of 'ideal_step'
    double exponent = floor(log10(ideal_step));
    double factor = pow(10.0, exponent);

    // Normalize ideal_step to the range [1, 10)
    double base = ideal_step / factor;
    double niceMultiplier;

    // 0.5, 1

    // Choose the largest candidate from {1, 2, 2.5, 5, 10} that is <= base.
    if (base >= 5)
        niceMultiplier = 5;
    else
        niceMultiplier = 1;

    /*if (base >= 10.0)
        niceMultiplier = 10.0;
    else if (base >= 5.0)
        niceMultiplier = 5.0;
    else if (base >= 2.5)
        niceMultiplier = 2.5;
    else if (base >= 2.0)
        niceMultiplier = 2.0;
    else
        niceMultiplier = 1.0;*/

    return niceMultiplier * factor;
}

double roundAxisValue(double v, double step)
{
    return floor(v / step) * step;
}

double ceilAxisValue(double v, double step)
{
    return ceil(v / step) * step;
}

double getAngle(DVec2 a, DVec2 b)
{
    return (b - a).angle();
}

void Viewport::printTouchInfo()
{
    double pinch_zoom_factor = camera.touchDist() / camera.pan_down_touch_dist;

    print() << "cam.position:               (" << camera.x << ",  " << camera.y << ")\n";
    print() << "cam.pan:                     (" << camera.pan_x << ",  " << camera.pan_y << ")\n";
    print() << "cam.rotation:              " << camera.rotation << "\n";
    print() << "cam.zoom:                  (" << camera.zoom_x << ", " << camera.zoom_y << ")\n\n";
    print() << "---------------------------------------------------------------\n";
    print() << "pan_down_touch_x           = " << camera.pan_down_touch_x << "\n";
    print() << "pan_down_touch_y           = " << camera.pan_down_touch_y << "\n";
    print() << "pan_down_touch_dist      = " << camera.pan_down_touch_dist << "\n";
    print() << "pan_down_touch_angle   = " << camera.pan_down_touch_angle << "\n";
    print() << "---------------------------------------------------------------\n";
    print() << "pan_beg_cam_x                = " << camera.pan_beg_cam_x << "\n";
    print() << "pan_beg_cam_y                = " << camera.pan_beg_cam_y << "\n";
    print() << "pan_beg_cam_zoom_x    = " << camera.pan_beg_cam_zoom_x << "\n";
    print() << "pan_beg_cam_zoom_y    = " << camera.pan_beg_cam_zoom_y << "\n";
    print() << "pan_beg_cam_angle        = " << camera.pan_beg_cam_angle << "\n";
    print() << "---------------------------------------------------------------\n";
    print() << "cam.touchAngle()        = " << camera.touchAngle() << "\n";
    print() << "cam.touchDist()         = " << camera.touchDist() << "\n";
    print() << "---------------------------------------------------------------\n";
    print() << "pinch_zoom_factor   = " << pinch_zoom_factor << "x\n";
    print() << "---------------------------------------------------------------\n";

    int i = 0;
    for (auto finger : camera.pressed_fingers)
    {
        print() << "Finger " << i << ": [id: " << finger.fingerId << "] - (" << finger.x << ", " << finger.y << ")\n";
        i++;
    }
}

void Viewport::drawWorldAxis(
    double axis_opacity,
    double grid_opacity,
    double text_opacity)
{
    save();

    // Fist, draw axis
    DVec2 stage_origin = camera.toStage(0, 0);
    DRect stage_rect = { 0, 0, width, height };

    // World quad
    DVec2 world_tl = camera.toWorld(0, 0);
    DVec2 world_tr = camera.toWorld(width, 0);
    DVec2 world_br = camera.toWorld(width, height);
    DVec2 world_bl = camera.toWorld(0, height);

    double world_w = world_br.x - world_tl.x;
    double world_h = world_br.y - world_tl.y;

    double stage_size = sqrt(width * width + height * height);
    double world_size = sqrt(world_w * world_w + world_h * world_h);
    double world_zoom = (world_size / stage_size);
    double angle = camera.rotation;

    // Get +positive Axis rays
    DRay axis_rayX = { stage_origin, angle + 0 };
    DRay axis_rayY = { stage_origin, angle + Math::HALF_PI };

    // Get axis intersection with viewport rect
    DVec2 negX_intersect, posX_intersect, negY_intersect, posY_intersect;
    bool x_axis_visible = Math::rayRectIntersection(&negX_intersect, &posX_intersect, stage_rect, axis_rayX);
    bool y_axis_visible = Math::rayRectIntersection(&negY_intersect, &posY_intersect, stage_rect, axis_rayY);

    // Convert to world coordinates
    DVec2 negX_intersect_world = camera.toWorld(negX_intersect);
    DVec2 posX_intersect_world = camera.toWorld(posX_intersect);
    DVec2 negY_intersect_world = camera.toWorld(negY_intersect);
    DVec2 posY_intersect_world = camera.toWorld(posY_intersect);

    // Draw with world coordinates, without line scaling
    camera.setTransformFilters(true, false, false, false);

    // Draw main axis lines
    setStrokeStyle(255, 255, 255, static_cast<int>(255.0 * axis_opacity));
    setLineWidth(1);

    beginPath();
    moveToSharp(negX_intersect_world.x, negX_intersect_world.y);
    lineToSharp(posX_intersect_world.x, posX_intersect_world.y);
    moveToSharp(negY_intersect_world.x, negY_intersect_world.y);
    lineToSharp(posY_intersect_world.x, posY_intersect_world.y);
    stroke();

    double aspect_ratio = width / height;

    // Draw axis ticks
    camera.setTransformFilters(false);
    double ideal_step_wx = ((width / aspect_ratio) / 7.0) / camera.zoom_x;
    double ideal_step_wy = (height / 7.0) / camera.zoom_y;
    double step_wx = roundAxisTickStep(ideal_step_wx);
    double step_wy = roundAxisTickStep(ideal_step_wy);
    /*double step, ideal_step;
    if (step_wx < step_wy)
    {
        ideal_step = ideal_step_wx;
        step = step_wy = step_wx;
    }
    else
    {
        ideal_step = ideal_step_wy;
        step = step_wx = step_wy;
    }*/

    //double upperTickStep = 

    //double prev_step = fmod(step, ideal_step *2.0);
    //double next_step = roundAxisTickStep(prev_step * 5.0);
    //double step_stretch = (ideal_step - prev_step) / (next_step - prev_step);
    double big_step_wx = step_wx * 5.0;
    double big_step_wy = step_wy * 5.0;




    /*fillText("ideal_step: " + QString::number(ideal_step), 0, 0);
    fillText("prev_step: " + QString::number(prev_step), 0, 20);
    fillText("next_step: " + QString::number(next_step), 0, 40);
    fillText("step_stretch: " + QString::number(step_stretch), 0, 60);*/

    double big_angle = 0;// step_stretch* M_PI * 2;
    double small_angle = (big_angle)+M_PI / 2;

    double big_visibility = 1;// sin(big_angle) / 2 + 0.5;
    double small_visibility = 0;// sin(small_angle) / 2 + 0.5;

    /*fillText("big_angle: " + QString::number((int)(big_angle*180.0/M_PI)), 0, 100);
    fillText("big_visibility: " + QString::number(big_visibility), 0, 120);

    fillText("small_angle: " + QString::number((int)(small_angle * 180.0 / M_PI)), 0, 160);
    fillText("small_visibility: " + QString::number(small_visibility), 0, 180);*/

    DVec2 x_perp_off = camera.toStageOffset(0, 6 * world_zoom);
    DVec2 x_perp_norm = camera.toStageOffset(0, 1).normalized();

    DVec2 y_perp_off = camera.toStageOffset(6 * world_zoom, 0);
    DVec2 y_perp_norm = camera.toStageOffset(1, 0).normalized();

    setTextAlign(TextAlign::ALIGN_CENTER);
    setTextBaseline(TextBaseline::BASELINE_MIDDLE);

    ///QNanoFont font(QNanoFont::FontId::DEFAULT_FONT_NORMAL);
    ///font.setPixelSize(12);
    ///setFont(font);

    double spacing = 8;

    // Get world bounds, regardless of rotation
    double world_minX = std::min({ world_tl.x, world_tr.x, world_br.x, world_bl.x });
    double world_maxX = std::max({ world_tl.x, world_tr.x, world_br.x, world_bl.x });
    double world_minY = std::min({ world_tl.y, world_tr.y, world_br.y, world_bl.y });
    double world_maxY = std::max({ world_tl.y, world_tr.y, world_br.y, world_bl.y });

    // Draw gridlines (big step)
    if (grid_opacity > 0)
    {

        setFillStyle(255, 255, 255, static_cast<int>(big_visibility * 5.0));

        bool offset = false;
        DVec2 p1, p2;

        for (double wy = ceilAxisValue(world_minY, big_step_wy); wy < world_maxY; wy += big_step_wy)
        {
            offset = !offset;

            int64_t iy = static_cast<int64_t>(wy / big_step_wy);

            DRay line_ray_y(camera.toStage(0, wy), angle);
            if (!Math::rayRectIntersection(&p1, &p2, stage_rect, line_ray_y)) break;

            for (double wx = ceilAxisValue(world_minX, big_step_wx); wx < world_maxX; wx += big_step_wx)
            {
                int64_t ix = static_cast<int64_t>(wx / big_step_wx);
                //int64_t i = ix + (iy % 2 ? 1 : 0);

                DRay line_ray_x(camera.toStage(wx, 0), angle + Math::HALF_PI);
                if (!Math::rayRectIntersection(&p1, &p2, stage_rect, line_ray_x)) break;

                if ((ix + iy) % 2 == 0)
                    fillRect(wx, wy, big_step_wx, big_step_wy);
            }

        }
    }

    if (axis_opacity > 0 && text_opacity > 0)
    {
        setStrokeStyle(255, 255, 255, static_cast<int>(255.0 * axis_opacity));
        setFillStyle(255, 255, 255, static_cast<int>(255.0 * text_opacity));
        beginPath();

        bool draw_text = (text_opacity > 0.01);

        // Draw x-axis labels
        for (double wx = ceilAxisValue(world_minX, step_wx); wx < world_maxX; wx += step_wx)
        {
            if (abs(wx) < 1e-9) continue;

            DVec2 stage_pos = camera.toStage(wx, 0);
            DVec2 txt_size = boundingBoxNumberScientific(wx).size();

            double txt_dist = (abs(cos(angle)) * txt_size.y + abs(sin(angle)) * txt_size.x) * 0.5 + spacing;

            DVec2 tick_anchor = stage_pos + x_perp_off + (x_perp_norm * txt_dist);

            moveToSharp(stage_pos - x_perp_off);
            lineToSharp(stage_pos + x_perp_off);
            if (draw_text) fillNumberScientific(wx, tick_anchor);
        }

        // Draw y-axis labels
        for (double wy = ceilAxisValue(world_minY, step_wy); wy < world_maxY; wy += step_wy)
        {
            if (abs(wy) < 1e-9) continue;

            DVec2 stage_pos = camera.toStage(0, wy);
            //DVec2 txt_size = measureText(txt);
            DVec2 txt_size = boundingBoxNumberScientific(wy).size();

            double txt_dist = (abs(cos(angle)) * txt_size.y + abs(sin(angle)) * txt_size.x) * 0.5 + spacing;

            DVec2 tick_anchor = stage_pos + y_perp_off + (y_perp_norm * txt_dist);

            moveToSharp(stage_pos - y_perp_off);
            lineToSharp(stage_pos + y_perp_off);
            if (draw_text) fillNumberScientific(wy, tick_anchor);
        }

        stroke();
    }

    restore();
    camera.setTransformFilters(true);
}

/// Layout

void Layout::expandCheck(size_t count)
{
    if (count > viewports.size())
        resize(count);
}

void Layout::add(int _viewport_index, int _grid_x, int _grid_y)
{
    Viewport* viewport = new Viewport(this, project->canvas, _viewport_index, _grid_x, _grid_y);
    viewports.push_back(viewport);
}

void Layout::resize(size_t viewport_count)
{
    if (targ_viewports_x <= 0 || targ_viewports_y <= 0)
    {
        // Spread proportionally
        rows = static_cast<int>(sqrt(viewport_count));
        cols = static_cast<int>(viewport_count) / rows;
    }
    else if (targ_viewports_y <= 0)
    {
        // Expand down
        cols = targ_viewports_x;
        rows = (int)ceil((float)viewport_count / (float)cols);
    }
    else if (targ_viewports_x <= 0)
    {
        // Expand right
        rows = targ_viewports_y;
        cols = (int)ceil((float)viewport_count / (float)rows);
    }

    // Expand rows down by default if not perfect fit
    if (viewport_count > rows * cols)
        rows++;

    int count = (cols * rows);

    for (int y = 0; y < rows; y++)
    {
        for (int x = 0; x < cols; x++)
        {
            int i = (y * cols) + x;
            if (i >= viewport_count)
            {
                goto break_nested;
            }

            if (i < viewports.size())
            {
                viewports[i]->viewport_grid_x = x;
                viewports[i]->viewport_grid_y = y;
            }
            else
            {
                Viewport* viewport = new Viewport(this, project->canvas, i, x, y);
                viewports.push_back(viewport);
            }
        }
    }
    break_nested:;

    // todo: Unmount remaining viewport sims
}

/// Project

void ProjectBase::configure(int _sim_uid, Canvas* _canvas, ImDebugLog* shared_log)
{
    DebugPrint("Project::configure");

    sim_uid = _sim_uid;
    canvas = _canvas;
    project_log = shared_log;

    viewports.project = this;

    started = false;
    paused = false;
}

void ProjectBase::_populateAllAttributes(bool show_ui)
{
    if (has_var_buffer)
    {
        bool showUI = ImGui::CollapsingHeader("Project", ImGuiTreeNodeFlags_DefaultOpen);

        if (showUI)
        {
            ImGui::PushID("project_section");
            _projectAttributes();
            ImGui::PopID();
        }

        ImGui::Dummy(ScaleSize(0, 8));
    }
	
    // Scene sections
    Layout& layout = currentLayout();
    auto& scenes = layout.scenes();
    for (SceneBase* scene : scenes)
    {
        if (!scene->has_var_buffer)
            continue;

        std::string sceneName = scene->name() + " " + std::to_string(scene->sceneIndex());
        std::string section_id = sceneName + "_section";
        bool showSceneUI = ImGui::CollapsingHeader(sceneName.c_str(), ImGuiTreeNodeFlags_DefaultOpen);
        if (showSceneUI)
        {
            // Allow Scene to populate inputs for section
            ImGui::PushID(section_id.c_str());
            scene->_sceneAttributes();
            ImGui::PopID();
        }
    }
}

void ProjectBase::_projectPrepare()
{
    DebugPrint("_projectPrepare()");
    logMessage("_projectPrepare()");
    viewports.clear();

    //ImGui::updatePointerValues();

    // Prepare project and create layout
    // Note: This is where old viewports get replaced
    scene_counter = 0;
    projectPrepare();
}

void ProjectBase::_projectStart()
{
    if (paused)
    {
        // If paused - just resume, don't restart
        paused = false;
        return;
    }

    done_single_process = false;

    // Prepare layout
    _projectPrepare();

    assert(viewports.count() >  0);

    // Update layout rects
    updateViewportRects();

    // Variable Buffer initializes itself *first*?
    //_updateLiveAttributes();

    // Call BasicProject::projectStart()
    projectStart();

    //cache.init("cache.bin");

    // Start project scenes
    for (SceneBase* scene : viewports.all_scenes)
    {
        ///scene->cache = &cache;
        scene->mouse = &mouse;
        ///scene->dt_scene_ma_list.clear();
        ///scene->dt_project_ma_list.clear();
        ///scene->dt_project_draw_ma_list.clear();
        scene->variableChangedClearMaps();

        scene->sceneStart();
    }

    // Mount to viewports
    for (Viewport* viewport : viewports)
    {
        viewport->scene->camera = &viewport->camera;
        viewport->scene->sceneMounted(viewport);
    }

    //for (SceneBase* scene : viewports.all_scenes)
    //    scene->_updateShadowAttributes();

    ///if (record_on_start)
    ///    startRecording();

    setProjectInfoState(ProjectInfo::ACTIVE);

    started = true;
}

void ProjectBase::_projectStop()
{
    projectStop();
    ///cache.finalize();

    ///if (record_manager.isRecording())
    ///    finalizeRecording();

    setProjectInfoState(ProjectInfo::INACTIVE);
    ///shaders_loaded = false;
    started = false;
}

void ProjectBase::_projectPause()
{
    paused = true;
}

void ProjectBase::_projectDestroy()
{
    setProjectInfoState(ProjectInfo::INACTIVE);
    projectDestroy();
}

void ProjectBase::updateViewportRects()
{
    DVec2 surface_size = surfaceSize();

    // Update viewport sizes
    double viewport_width = surface_size.x / viewports.cols;
    double viewport_height = surface_size.y / viewports.rows;

    // Update viewport rects
    for (Viewport* viewport : viewports)
    {
        viewport->x = viewport->viewport_grid_x * viewport_width;
        viewport->y = viewport->viewport_grid_y * viewport_height;
        viewport->width = viewport_width - 1;
        viewport->height = viewport_height - 1;
        viewport->camera.viewport_w = viewport_width - 1;
        viewport->camera.viewport_h = viewport_height - 1;
    }
}

DVec2 ProjectBase::surfaceSize()
{
    double w;
    double h;

    // Determine surface size to draw to (depends on if recording or not)
    //if (!record_manager.isRecording() || window_capture)
    {
        w = canvasWidth();
        h = canvasHeight();
    }
    //else
    //{
    //    IVec2 record_resolution = options->getRecordResolution();
    //
    //    w = record_resolution.x;
    //    h = record_resolution.y;
    //}

    return Vec2(w, h);
}

void ProjectBase::_projectProcess()
{
    auto frame_dt = std::chrono::steady_clock::now() - last_frame_time;
    dt_frameProcess = static_cast<int>(
        std::chrono::duration_cast<std::chrono::milliseconds>(frame_dt).count()
    );
    
    last_frame_time = std::chrono::steady_clock::now();

    updateViewportRects();

    bool did_first_process = false;

    // Determine whether to process project this frame or not (depends on recording status)
    //if (!record_manager.attachingEncoder() && !record_manager.encoderBusy())
    {
        // Allow panning/zooming, even when paused
        for (Viewport* viewport : viewports)
        {
            double viewport_mx = mouse.client_x - viewport->x;
            double viewport_my = mouse.client_y - viewport->y;

            if (viewport_mx >= 0 && viewport_my >= 0 &&
                viewport_mx <= viewport->width && viewport_my <= viewport->height)
            {
                Camera& cam = viewport->camera;

                DVec2 world_mouse = cam.toWorld(viewport_mx, viewport_my);
                mouse.viewport = viewport;
                mouse.stage_x = viewport_mx;
                mouse.stage_y = viewport_my;
                mouse.world_x = world_mouse.x;
                mouse.world_y = world_mouse.y;
                viewport->camera.panZoomProcess();
            }
        }

        if (!paused)
        {
            auto project_t0 = std::chrono::steady_clock::now();

            // Process each scene
            for (SceneBase* scene : viewports.all_scenes)
            {
                scene->dt_call_index = 0;
                auto scene_t0 = std::chrono::steady_clock::now();

                scene->mouse = &mouse;
                scene->sceneProcess();

                auto scene_dt = std::chrono::steady_clock::now() - scene_t0;
                scene->dt_sceneProcess = static_cast<int>(
                    std::chrono::duration_cast<std::chrono::milliseconds>(scene_dt).count()
                );
            }


            // Allow project to handle process on each Viewport
            for (Viewport* viewport : viewports)
            {
                //viewport->camera.panZoomProcess();
                viewport->scene->camera = &viewport->camera;

                ///viewport->just_resized =
                ///    (viewport->width != viewport->old_width) ||
                ///    (viewport->height != viewport->old_height);

                viewport->scene->viewportProcess(viewport);

                ///viewport->old_width = viewport->width;
                ///viewport->old_height = viewport->height;
            }

            // Post-Process each scene
            for (SceneBase* scene : viewports.all_scenes)
                scene->variableChangedUpdateCurrent();
            
            auto project_dt = std::chrono::steady_clock::now() - project_t0;
            dt_projectProcess = static_cast<int>(
                std::chrono::duration_cast<std::chrono::milliseconds>(project_dt).count()
            );

            // Prepare to encode the next frame
            ///encode_next_paint = true;
            if (!done_single_process)
                did_first_process = true;
        }
    }

    if (did_first_process)
        done_single_process = true;
}

void ProjectBase::_projectDraw()
{
    if (!done_single_process) return;
    if (!started) return;
    

    ///if (!shaders_loaded)
    ///{
    ///    _loadShaders();
    ///    shaders_loaded = true;
    ///}

    Canvas* ctx = canvas;
    DVec2 surface_size = surfaceSize();

    ctx->setFillStyle(10, 10, 15);
    ctx->fillRect(0, 0, surface_size.x, surface_size.y);

    ctx->setFillStyle(255,255,255);
    ctx->setStrokeStyle(255,255,255);

    ///timer_projectDraw.start();

    // Draw each viewport
    int i = 0;
    for (Viewport* viewport : viewports)
    {
        ctx->setClipRect(viewport->x, viewport->y, viewport->width, viewport->height);
        ctx->save();

        // Move to viewport position
        ctx->translate(floor(viewport->x), floor(viewport->y));

        // Attach QNanoPainter for viewport draw operations
        ///viewport->painter = p;

        // Set default transform to "World"
        viewport->camera.worldTransform();

        // Draw Scene to Viewport
        viewport->scene->camera = &viewport->camera;
        viewport->draw();

        ctx->restore();
        ctx->resetClipping();
    }

    ///dt_projectDraw = timer_projectDraw.elapsed();

    // Draw viewport splitters
    for (Viewport* viewport : viewports)
    {
        ctx->setLineWidth(6);
        ctx->setStrokeStyle(30, 30, 40);
        ctx->beginPath();

        // Draw vert line
        if (viewport->viewport_grid_x < viewports.cols - 1)
        {
            double line_x = floor(viewport->x + viewport->width) + 0.5;
            ctx->moveTo(line_x, viewport->y);
            ctx->lineTo(line_x, viewport->y + viewport->height + 1);
        }

        // Draw horiz line
        if (viewport->viewport_grid_y < viewports.rows - 1)
        {
            double line_y = floor(viewport->y + viewport->height) + 0.5;
            ctx->moveTo(viewport->x + viewport->width + 1, line_y);
            ctx->lineTo(viewport->x, line_y);
        }

        ctx->stroke();
    }
}

void ProjectBase::_onEvent(SDL_Event& e)
{
    Event event(e);

    // Track "cursor" position (pointer/first touch)
    switch (e.type)
    {
        case SDL_MOUSEMOTION:
        {
            mouse.client_x = e.motion.x;
            mouse.client_y = e.motion.y;
        }
        break;

        case SDL_FINGERMOTION:
        {
            mouse.client_x = (double)(e.tfinger.x * (float)Platform()->drawable_width());
            mouse.client_y = (double)(e.tfinger.y * (float)Platform()->drawable_height());
        }
        break;

        ///case SDL_MOUSEWHEEL:
        ///{
        ///    mouse.scroll_delta = e.wheel.
        ///}
        ///break;
    }

    // Track "focused" and "hovered" viewports
    switch (e.type)
    {
        case SDL_MOUSEBUTTONDOWN:
        case SDL_FINGERDOWN:
        {
            bool captured = false;
            for (Viewport* ctx : viewports)
            {
                if (ctx->viewportRect().hitTest(mouse.client_x, mouse.client_y))
                {
                    focused_ctx = ctx;
                    captured = true;
                    break;
                }
            }
            if (!captured)
                focused_ctx = nullptr;
        }
        break;

        case SDL_MOUSEMOTION:
        case SDL_FINGERMOTION:
        {
            bool captured = false;
            for (Viewport* ctx : viewports)
            {
                if (ctx->viewportRect().hitTest(mouse.client_x, mouse.client_y))
                {
                    hovered_ctx = ctx;
                    captured = true;
                    break;
                }
            }
            if (!captured)
                hovered_ctx = nullptr;
        }
        break;
    }


    // Track fingers
    if (Platform()->is_mobile())
    {
        // Does this finger already have an owner? Attach it to the event
        Viewport* owner_ctx = nullptr;
        
        // Support both single-finger pan & 2 finger transform
        switch (e.type)
        {
        case SDL_FINGERDOWN:
        {
            owner_ctx = focused_ctx;

            // Add pressed finger
            FingerInfo info;
            info.owner_ctx = focused_ctx;
            info.fingerId = e.tfinger.fingerId;
            info.x = event.finger_x();
            info.y = event.finger_y();
            pressed_fingers.push_back(info);

            // Remember mouse-down ctx for mouse release/motion
            event.setOwnerViewport(owner_ctx);
        }
        break;

        case SDL_FINGERMOTION:
        {
            for (size_t i = 0; i < pressed_fingers.size(); i++)
            {
                FingerInfo& info = pressed_fingers[i];
                if (info.fingerId == e.tfinger.fingerId)
                {
                    owner_ctx = info.owner_ctx;
                    break;
                }
            }

            event.setOwnerViewport(owner_ctx);
        }
        break;

        case SDL_FINGERUP:
        {
            for (size_t i = 0; i < pressed_fingers.size(); i++)
            {
                FingerInfo& info = pressed_fingers[i];
                if (info.fingerId == e.tfinger.fingerId)
                {
                    owner_ctx = info.owner_ctx;
                    break;
                }
            }

            // Erase lifted finger
            for (size_t i = 0; i < pressed_fingers.size(); i++)
            {
                FingerInfo& info = pressed_fingers[i];
                if (info.fingerId == e.tfinger.fingerId)
                {
                    pressed_fingers.erase(pressed_fingers.begin() + i);
                    break;
                }
            }

            event.setOwnerViewport(owner_ctx);
        }
        break;
        }
    }
    else
    {
        switch (e.type)
        {
            case SDL_MOUSEBUTTONDOWN:
            case SDL_MOUSEBUTTONUP:
            case SDL_MOUSEMOTION:
            {
                event.setOwnerViewport(focused_ctx);
            }
            break;
            case SDL_MOUSEWHEEL:
            {
                event.setOwnerViewport(hovered_ctx);
            }
            break;
        }
    }


    event.setFocusedViewport(focused_ctx);
    event.setHoveredViewport(hovered_ctx);

    onEvent(event);

    for (SceneBase* scene : viewports.all_scenes)
        scene->_onEvent(event);
}



void ProjectBase::setProjectInfoState(ProjectInfo::State state)
{
    getProjectInfo()->state = state;
}

Layout& ProjectBase::newLayout()
{
    viewports.clear();
    return viewports;
}

Layout& ProjectBase::newLayout(int targ_viewports_x, int targ_viewports_y)
{
    viewports.clear();
    viewports.setSize(targ_viewports_x, targ_viewports_y);

    return viewports;
}

void SceneBase::logMessage(const char* fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    project->project_log->vlog(fmt, ap);
    va_end(ap);
}

void ProjectBase::logMessage(const char* fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    project_log->vlog(fmt, ap);
    va_end(ap);
}

void SceneBase::logClear()
{
    project->project_log->clear();
}

void ProjectBase::logClear()
{
    project_log->clear();
}


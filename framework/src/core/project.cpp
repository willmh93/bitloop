#include <bitloop/core/project.h>
#include <bitloop/core/main_window.h>

BL_BEGIN_NS

/// =======================
/// ======== Scene ========
/// =======================

/*TouchState::TouchState(const Event& e, const std::vector<FingerInfo>& _fingers) //: event(e)
{
    for (const FingerInfo& f : _fingers)
        fingers.push_back(f);

    Viewport* viewport = e.ctx_owner();
    Camera& cam = viewport->camera;

    for (SceneFingerInfo& f : fingers)
    {
        double viewport_mx = f.x - viewport->posX();
        double viewport_my = f.y - viewport->posY();

        if (cam.isPanning() ||
            (viewport_mx >= 0 &&
            viewport_my >= 0 &&
            viewport_mx <= viewport->width() &&
            viewport_my <= viewport->height()))
        {

            DVec2 world_mouse = cam.toWorld(viewport_mx, viewport_my);
            f.viewport = viewport;
            f.stage_x = viewport_mx;
            f.stage_y = viewport_my;
            f.world_x = world_mouse.x;
            f.world_y = world_mouse.y;
        }
    }
}*/

void SceneBase::registerMount(Viewport* viewport)
{
    mounted_to_viewports.push_back(viewport);
    // Mounted to Viewport: viewport->viewportIndex()

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
        // Erasing project scene from viewport
        all_scenes.erase(scene_it);
    }

    // Only unmount from viewport if the layout includes the viewport (which it should)
    auto viewport_it = std::find(mounted_to_viewports.begin(), mounted_to_viewports.end(), viewport);
    if (viewport_it != mounted_to_viewports.end())
    {
        // Unmounting scene from viewport
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

bool bl::SceneBase::ownsEvent(const Event& e)
{
    return e.ctx_owner() && e.ctx_owner()->scene == this;
}

void SceneBase::pollEvents()
{
    project_worker()->pollEvents(false);
}

void SceneBase::pullDataFromShadow()
{
    project_worker()->pullDataFromShadow();
}

bool SceneBase::handleWorldNavigation(Event e, bool single_touch_pan, bool zoom_anchor_mouse)
{
    if (e.ctx_focused())
    {
        return e.ctx_focused()->camera.handleWorldNavigation(e, single_touch_pan, zoom_anchor_mouse);
        //return true;
    }
    return false;
}

/*void SceneBase::handleWorldNavigation(Event e, bool single_touch_pan)
{
    if (e.ctx_focused())
        e.ctx_focused()->camera.handleWorldNavigation(e, single_touch_pan);
}*/

double SceneBase::frame_dt(int average_samples) const
{
    if (dt_call_index >= dt_scene_ma_list.size())
        dt_scene_ma_list.push_back(Math::MovingAverage::MA<double>(average_samples));

    auto& ma = dt_scene_ma_list[dt_call_index++];
    return ma.push(project->dt_frameProcess);
}

double SceneBase::scene_dt(int average_samples) const
{
    if (dt_call_index >= dt_scene_ma_list.size())
        dt_scene_ma_list.push_back(Math::MovingAverage::MA<double>(average_samples));

    auto& ma = dt_scene_ma_list[dt_call_index++];
    return ma.push(dt_sceneProcess);
}

double SceneBase::project_dt(int average_samples) const
{
    if (dt_process_call_index >= dt_project_ma_list.size())
        dt_project_ma_list.push_back(Math::MovingAverage::MA<double>(average_samples));

    auto& ma = dt_project_ma_list[dt_process_call_index++];
    return ma.push(project->dt_projectProcess);
}

bool SceneBase::isRecording() const
{
    return main_window()->getRecordManager()->isRecording();
}

void SceneBase::queueBeginRecording()
{
    main_window()->queueBeginRecording();
}

void SceneBase::queueEndRecording()
{
    main_window()->queueEndRecording();
}

void SceneBase::captureFrame(bool b)
{
    main_window()->captureFrame(b);
}

bool SceneBase::capturedLastFrame()
{
    return main_window()->capturedLastFrame();
}

/// ==========================
/// ======== Viewport ========
/// ==========================

Viewport::Viewport(Layout* layout, int viewport_index, int grid_x, int grid_y) :
    layout(layout),
    viewport_index(viewport_index),
    viewport_grid_x(grid_x),
    viewport_grid_y(grid_y)
{
    
    
    

    camera.viewport = this;
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
            scene->sceneDestroy();
            delete scene;
        }
    }
}

void Viewport::draw()
{
    // Set defaults
    setFont(getDefaultFont());
    setTextAlign(TextAlign::ALIGN_LEFT);
    setTextBaseline(TextBaseline::BASELINE_TOP);

    // Snapshot default transformation (unscaled unrotated top-left viewport)
    default_viewport_transform = currentTransform();

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
        fillText(line, 5.0, 5.0 + (line_index++ * getGlobalScale() * 16.0f));

    camera.restoreCameraTransform();
    restore();
}

void Viewport::overlay()
{
    // Set defaults
    setFont(getDefaultFont());
    setTextAlign(TextAlign::ALIGN_LEFT);
    setTextBaseline(TextBaseline::BASELINE_TOP);

    // Snapshot default transformation (unscaled unrotated top-left viewport)
    default_viewport_transform = currentTransform();

    print_stream.str("");
    print_stream.clear();

    // Draw mounted project to this viewport
    scene->camera = &camera;
    scene->viewportOverlay(this);

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
        fillText(line, 5.0, 5.0 + (line_index++ * getGlobalScale() * 16.0f));

    camera.restoreCameraTransform();
    restore();
}

void Viewport::printTouchInfo()
{
    double pinch_zoom_factor = camera.touchDist() / camera.panDownTouchDist();

    setFont(getDefaultFont());
    print() << "cam.position          = (" << camera.x() << ",  " << camera.y() << ")\n";
    print() << "cam.pan               = (" << camera.panX() << ",  " << camera.panY() << ")\n";
    print() << "cam.rotation          = " << camera.rotation() << "\n";
    print() << "cam.zoom              = (" << camera.zoomX() << ", " << camera.zoomY() << ")\n\n";
    print() << "--------------------------------------------------------------\n";
    print() << "pan_down_touch_x      = " << camera.panDownTouchX() << "\n";
    print() << "pan_down_touch_y      = " << camera.panDownTouchY() << "\n";
    print() << "pan_down_touch_dist   = " << camera.panDownTouchDist() << "\n";
    print() << "pan_down_touch_angle  = " << camera.panDownTouchAngle() << "\n";
    print() << "--------------------------------------------------------------\n";
    print() << "pan_beg_cam_x         = " << (double)camera.panBegCamX() << "\n";
    print() << "pan_beg_cam_y         = " << (double)camera.panBegCamY() << "\n";
    print() << "pan_beg_cam_zoom_x    = " << (double)camera.panBegCamZoomX() << "\n";
    print() << "pan_beg_cam_zoom_y    = " << (double)camera.panBegCamZoomY() << "\n";
    print() << "pan_beg_cam_angle     = " << camera.panBegCamAngle() << "\n";
    print() << "--------------------------------------------------------------\n";
    print() << "cam.touchAngle()      = " << camera.touchAngle() << "\n";
    print() << "cam.touchDist()       = " << camera.touchDist() << "\n";
    print() << "--------------------------------------------------------------\n";
    print() << "pinch_zoom_factor     = " << pinch_zoom_factor << "x\n";
    print() << "--------------------------------------------------------------\n";

    int i = 0;
    for (auto finger : camera.pressedFingers())
    {
        print() << "Finger " << i << ": [id: " << finger.fingerId << "] - (" << finger.x << ", " << finger.y << ")\n";
        i++;
    }
}


/// ========================
/// ======== Layout ========
/// ========================

void SimSceneList::mountTo(Layout& viewports) 
{
    for (size_t i = 0; i < size(); i++)
        at(i)->mountTo(viewports[i]);
}

void Layout::expandCheck(size_t count)
{
    if (count > viewports.size())
        resize(count);
}

void Layout::add(int _viewport_index, int _grid_x, int _grid_y)
{
    Viewport* viewport = new Viewport(this, _viewport_index, _grid_x, _grid_y);
    viewports.push_back(viewport);
}

void Layout::resize(size_t viewport_count)
{
    if (targ_viewports_x <= 0 || targ_viewports_y <= 0)
    {
        // Spread proportionally
        rows = static_cast<int>(std::sqrt(viewport_count));
        cols = static_cast<int>(viewport_count) / rows;
    }
    else if (targ_viewports_y <= 0)
    {
        // Expand down
        cols = targ_viewports_x;
        rows = (int)std::ceil((float)viewport_count / (float)cols);
    }
    else if (targ_viewports_x <= 0)
    {
        // Expand right
        rows = targ_viewports_y;
        cols = (int)std::ceil((float)viewport_count / (float)rows);
    }

    // Expand rows down by default if not perfect fit
    if (viewport_count > rows * cols)
        rows++;

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
                Viewport* viewport = new Viewport(this, i, x, y);
                viewports.push_back(viewport);
            }
        }
    }
    break_nested:;

    // todo: Unmount remaining viewport sims
}

/// =========================
/// ======== Project ========
/// =========================

void ProjectBase::configure(int _sim_uid, Canvas* _canvas, Canvas* _overlay, ImDebugLog* shared_log)
{
    blPrint() << "Project::configure";

    sim_uid = _sim_uid;
    canvas = _canvas;
    overlay = _overlay;
    project_log = shared_log;

    viewports.project = this;

    started = false;
    paused = false;
}

void ProjectBase::_populateAllAttributes()
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

        ImGui::Dummy(scale_size(0, 8));
    }
	
    // Scene sections
    Layout& layout = currentLayout();
    auto& scenes = layout.scenes();
    for (SceneBase* scene : scenes)
    {
        //if (!scene->has_var_buffer)
        //    continue;

        std::string sceneName = scene->name() + " " + std::to_string(scene->sceneIndex());
        std::string section_id = sceneName + "_section";
        bool showSceneUI = (scenes.size() == 1) || ImGui::CollapsingHeader(sceneName.c_str(), ImGuiTreeNodeFlags_DefaultOpen);
        if (showSceneUI)
        {
            // Allow Scene to populate inputs for section
            ImGui::PushID(section_id.c_str());

            ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.5f, 0.2f, 0.2f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.6f, 0.3f, 0.3f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(7.0f, 0.4f, 0.4f, 1.0f));

            scene->_sceneAttributes();

            ImGui::PopStyleColor(3);
            ImGui::PopID();
        }
    }
}

void ProjectBase::_projectPrepare()
{
    viewports.clear();

    // Prepare project and create layout
    // Note: This is where old viewports get replaced
    scene_counter = 0;
    projectPrepare(newLayout());
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
    updateViewportRects();
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
        scene->clearCurrent();

        scene->sceneStart();
    }

    // Mount to viewports
    for (Viewport* viewport : viewports)
    {
        viewport->usePainter(canvas->getPainterContext());

        Camera::active = &viewport->camera;
        viewport->scene->camera = &viewport->camera;

        viewport->scene->sceneMounted(viewport);
    }

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

DVec2 ProjectBase::surfaceSize()
{
    double w;
    double h;

    // Determine surface size to draw to (depends on if recording or not)
    //if (!record_manager.isRecording() || window_capture)
    {
        w = fboWidth();
        h = fboHeight();
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

void ProjectBase::updateViewportRects()
{
    assert(viewports.count() > 0);

    DVec2 surface_size = surfaceSize();

    // Update viewport sizes
    double viewport_width = surface_size.x / viewports.cols;
    double viewport_height = surface_size.y / viewports.rows;

    // Update viewport rects
    for (Viewport* viewport : viewports)
    {
        viewport->x = viewport->viewport_grid_x * viewport_width;
        viewport->y = viewport->viewport_grid_y * viewport_height;
        viewport->w = viewport_width - 1;
        viewport->h = viewport_height - 1;
        viewport->camera.viewport_w = viewport_width - 1;
        viewport->camera.viewport_h = viewport_height - 1;
    }
}

void ProjectBase::_projectProcess()
{
    auto frame_dt = std::chrono::steady_clock::now() - last_frame_time;
    dt_frameProcess = static_cast<double>(std::chrono::duration_cast<std::chrono::milliseconds>(frame_dt).count());
    
    last_frame_time = std::chrono::steady_clock::now();

    updateViewportRects();

    bool did_first_process = false;

    // Determine whether to process project this frame or not (depends on recording status)
    //if (!record_manager.attachingEncoder() && !record_manager.encoderBusy())
    {
        // Allow panning/zooming, even when paused
        for (Viewport* viewport : viewports)
        {
            viewport->usePainter(canvas->getPainterContext());
            Camera::active = &viewport->camera;
            viewport->scene->camera = &viewport->camera;

            double viewport_mx = mouse.client_x - viewport->x;
            double viewport_my = mouse.client_y - viewport->y;

            Camera& cam = viewport->camera;

            if (cam.panning ||
                (viewport_mx >= 0 &&
                 viewport_my >= 0 &&
                 viewport_mx <= viewport->w && 
                 viewport_my <= viewport->h))
            {

                DVec2 world_mouse = cam.toWorld(viewport_mx, viewport_my);
                mouse.viewport = viewport;
                mouse.stage_x = viewport_mx;
                mouse.stage_y = viewport_my;
                mouse.world_x = world_mouse.x;
                mouse.world_y = world_mouse.y;
                viewport->camera.panZoomProcess();
            }

            // Immediately update world -> stage transformation matrix
            cam.updateCameraMatrix();
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

                viewport->usePainter(canvas->getPainterContext());

                Camera::active = &viewport->camera;
                viewport->scene->camera = &viewport->camera;

                viewport->just_resized =
                    (viewport->w != viewport->old_w) ||
                    (viewport->h != viewport->old_h);

                double dt;
                if (main_window()->getRecordManager()->isRecording())
                    dt = 1.0 / main_window()->getRecordManager()->fps();
                else
                    dt = dt_frameProcess / 1000.0;

                viewport->scene->viewportProcess(viewport, dt);

                viewport->old_w = viewport->w;
                viewport->old_h = viewport->h;
            }

            // Post-Process each scene
            for (SceneBase* scene : viewports.all_scenes)
                scene->updateCurrent();
            
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

    //Canvas* ctx = canvas;
    DVec2 surface_size = surfaceSize();

    // Draw background
    canvas->setFillStyle(10, 10, 15);
    canvas->fillRect(0, 0, surface_size.x, surface_size.y);

    canvas->setFillStyle(255,255,255);
    canvas->setStrokeStyle(255,255,255);

    canvas->setFontSize(16.0f);

    ///timer_projectDraw.start();

    // Draw each viewport
    for (Viewport* viewport : viewports)
    {
        // Reuse derived painter for the primary canvas context
        viewport->usePainter(canvas->getPainterContext());

        Camera::active = &viewport->camera;
        viewport->scene->camera = &viewport->camera;

        canvas->setClipRect(viewport->x, viewport->y, viewport->w, viewport->h);
        canvas->save();

        // Starting viewport position
        canvas->translate(std::floor(viewport->x), std::floor(viewport->y));

        // Set default transform to "World"
        viewport->camera.worldTransform();

        // Draw Scene to Viewport

        viewport->draw();

        // Restore initial canvas transform
        canvas->restore();
        canvas->resetClipping();
    }

    ///dt_projectDraw = timer_projectDraw.elapsed();

    // Draw viewport splitters
    for (Viewport* viewport : viewports)
    {
        canvas->setLineWidth(6);
        canvas->setStrokeStyle(30, 30, 40);
        canvas->beginPath();

        // Draw vert line
        if (viewport->viewport_grid_x < viewports.cols - 1)
        {
            double line_x = std::floor(viewport->x + viewport->w) + 0.5;
            canvas->moveTo(line_x, viewport->y);
            canvas->lineTo(line_x, viewport->y + viewport->h + 1);
        }

        // Draw horiz line
        if (viewport->viewport_grid_y < viewports.rows - 1)
        {
            double line_y = std::floor(viewport->y + viewport->h) + 0.5;
            canvas->moveTo(viewport->x + viewport->w + 1, line_y);
            canvas->lineTo(viewport->x, line_y);
        }

        canvas->stroke();
    }
}

void ProjectBase::_projectOverlay()
{
    return;

    if (!done_single_process) return;
    if (!started) return;

    DVec2 surface_size = surfaceSize();

    //overlay->setFillStyle(10, 10, 15);
    //overlay->fillRect(0, 0, surface_size.x, surface_size.y);

    overlay->setFillStyle(255, 255, 255);
    overlay->setStrokeStyle(255, 255, 255);

    overlay->setFontSize(16.0f);

    ///timer_projectDraw.start();

    // Draw each viewport
    for (Viewport* viewport : viewports)
    {
        // Reuse derived painter for the overlay context
        viewport->usePainter(overlay->getPainterContext());

        Camera::active = &viewport->camera;
        viewport->scene->camera = &viewport->camera;

        overlay->setClipRect(viewport->x, viewport->y, viewport->w, viewport->h);
        overlay->save();

        // Starting viewport position
        overlay->translate(std::floor(viewport->x), std::floor(viewport->y));

        // Set default transform to "World"
        viewport->camera.stageTransform();

        // Draw Scene to Viewport
        viewport->overlay();

        // Restore initial canvas transform
        overlay->restore();
        overlay->resetClipping();
    }
}

void ProjectBase::_clearEventQueue()
{
    for (SceneBase* scene : viewports.all_scenes)
    {
        scene->input.clear();
    }
}

void ProjectBase::_onEvent(SDL_Event& e)
{
    // If viewport not hovered (e.g. Mouse over a popup window),
    // ignore mouse/finger down events to avoid stealing focus
    switch (e.type)
    {
    case SDL_EVENT_MOUSE_BUTTON_DOWN:
    case SDL_EVENT_FINGER_DOWN:
    {
        if (!main_window()->viewportHovered())
            return;
    } break;
    }

    // Wrap SDL event
    Event event(e);

    TouchInput s;

    // Track "cursor" position
    switch (e.type)
    {
        // Mouse
        case SDL_EVENT_MOUSE_BUTTON_DOWN:
        {
            mouse.pressed = true;
            mouse.client_x = e.motion.x;
            mouse.client_y = e.motion.y;

        } break;
        case SDL_EVENT_MOUSE_BUTTON_UP:
        {
            mouse.pressed = false;
            mouse.client_x = e.motion.x;
            mouse.client_y = e.motion.y;
        } break;
        case SDL_EVENT_MOUSE_MOTION:
        {
            mouse.client_x = e.motion.x;
            mouse.client_y = e.motion.y;
        } break;

        // Touch
        case SDL_EVENT_FINGER_DOWN:
        case SDL_EVENT_FINGER_UP:
        case SDL_EVENT_FINGER_MOTION:
        {
            mouse.client_x = (double)(e.tfinger.x * (float)fboWidth());
            mouse.client_y = (double)(e.tfinger.y * (float)fboHeight());
        } break;
    }

    // Detect which viewport we're "focusing" / "hovering" on (to attach to event)
    switch (e.type)
    {
        case SDL_EVENT_MOUSE_BUTTON_DOWN:
        case SDL_EVENT_FINGER_DOWN:
        {
            // Update "ctx_focused" when we touch/click
            bool captured = false;
            for (Viewport* ctx : viewports)
            {
                ctx->usePainter(canvas->getPainterContext());
                Camera::active = &ctx->camera;
                ctx->scene->camera = &ctx->camera;

                if (ctx->viewportRect().hitTest(mouse.client_x, mouse.client_y))
                {
                    ctx_focused = ctx;
                    captured = true;
                    break;
                }
            }
            if (!captured)
                ctx_focused = nullptr;
        } break;
        case SDL_EVENT_MOUSE_MOTION:
        case SDL_EVENT_FINGER_MOTION:
        {
            // Detect which viewport we're "hovering" over
            bool captured = false;
            for (Viewport* ctx : viewports)
            {
                ctx->usePainter(canvas->getPainterContext());
                Camera::active = &ctx->camera;
                ctx->scene->camera = &ctx->camera;

                if (ctx->viewportRect().hitTest(mouse.client_x, mouse.client_y))
                {
                    ctx_hovered = ctx;
                    captured = true;
                    break;
                }
            }
            if (!captured)
                ctx_hovered = nullptr;
        } break;
    }


    // Update pressed_fingers
    {
        if (platform()->is_mobile())
        {
            // Does this finger already have an "owner" viewport?
            // If so, loop over fingers and restore it for 
            Viewport* ctx_owner = nullptr;

            // Support both single-finger pan & 2 finger transform
            switch (e.type)
            {
            case SDL_EVENT_FINGER_DOWN:
            {
                ctx_owner = ctx_focused;
                PointerEvent pointer_event(event);

                // Add pressed finger
                FingerInfo info;
                info.ctx_owner = ctx_focused;
                info.fingerId = e.tfinger.fingerID;
                info.x = pointer_event.x();
                info.y = pointer_event.y();
                pressed_fingers.push_back(info);

                // Remember touched-down ctx for touch release/motion event to use same owner
                event.setOwnerViewport(ctx_owner);
            } break;
            case SDL_EVENT_FINGER_MOTION:
            {
                for (size_t i = 0; i < pressed_fingers.size(); i++)
                {
                    FingerInfo& info = pressed_fingers[i];
                    if (info.fingerId == e.tfinger.fingerID)
                    {
                        ctx_owner = info.ctx_owner;
                        break;
                    }
                }

                event.setOwnerViewport(ctx_owner);
            } break;
            case SDL_EVENT_FINGER_UP:
            {
                for (size_t i = 0; i < pressed_fingers.size(); i++)
                {
                    FingerInfo& info = pressed_fingers[i];
                    if (info.fingerId == e.tfinger.fingerID)
                    {
                        ctx_owner = info.ctx_owner;
                        break;
                    }
                }

                // Erase lifted finger
                for (size_t i = 0; i < pressed_fingers.size(); i++)
                {
                    FingerInfo& info = pressed_fingers[i];
                    if (info.fingerId == e.tfinger.fingerID)
                    {
                        pressed_fingers.erase(pressed_fingers.begin() + i);
                        break;
                    }
                }

                event.setOwnerViewport(ctx_owner);
            } break;
            }
        }
        else
        {
            Viewport* ctx_owner = nullptr;

            switch (e.type)
            {
            case SDL_EVENT_MOUSE_BUTTON_DOWN:
            {
                ctx_owner = ctx_focused;
                PointerEvent pointer_event(event);

                // Add pressed finger
                FingerInfo info;
                info.ctx_owner = ctx_focused;
                info.fingerId = 0;
                info.x = pointer_event.x();
                info.y = pointer_event.y();
                pressed_fingers.push_back(info);

                // Remember touched-down ctx for touch release/motion event to use same owner
                event.setOwnerViewport(ctx_owner);
            } break;
            case SDL_EVENT_MOUSE_BUTTON_UP:
            {
                for (size_t i = 0; i < pressed_fingers.size(); i++)
                {
                    FingerInfo& info = pressed_fingers[i];
                    if (info.fingerId == 0)
                    {
                        ctx_owner = info.ctx_owner;
                        break;
                    }
                }

                pressed_fingers.clear();
                event.setOwnerViewport(ctx_owner);
            } break;
            case SDL_EVENT_MOUSE_MOTION:
            {
                for (size_t i = 0; i < pressed_fingers.size(); i++)
                {
                    FingerInfo& info = pressed_fingers[i];
                    if (info.fingerId == 0)
                    {
                        ctx_owner = info.ctx_owner;
                        break;
                    }
                }

                event.setOwnerViewport(ctx_focused);
            } break;
            case SDL_EVENT_MOUSE_WHEEL:
            {
                event.setOwnerViewport(ctx_hovered);
            } break;
            }
        }
    }
    
    event.setFocusedViewport(ctx_focused);
    event.setHoveredViewport(ctx_hovered);

    if (event.ctx_owner())
    {
        event.ctx_owner()->scene->input.addEvent(event, pressed_fingers);
    }

    onEvent(event);

    for (SceneBase* scene : viewports.all_scenes)
    {
        // Keep specialized handlers onMouseDown/onKeyDown(etc) pre-filtered, 
        // but keep onEvent generic so users can retain full control.
        scene->_onEvent(event);
    }
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

bool ProjectBase::isRecording() const
{
    return main_window()->getRecordManager()->isRecording();
}

void SceneBase::logMessage(std::string_view fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    project->project_log->vlog(fmt.data(), ap);
    va_end(ap);
}

void ProjectBase::pullDataFromShadow()
{
    project_worker()->pullDataFromShadow();
}

void ProjectBase::pollEvents()
{
    project_worker()->pollEvents(false);
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

BL_END_NS

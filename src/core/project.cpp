#include "project.h"
#include "main_window.h"

/// =======================
/// ======== Scene ========
/// =======================

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

void SceneBase::pollEvents()
{
    ProjectWorker::instance()->pollEvents(false);
}

void SceneBase::pullDataFromShadow()
{
    ProjectWorker::instance()->pullDataFromShadow();
}

void SceneBase::handleWorldNavigation(Event e, bool single_touch_pan)
{
    if (e.ctx_focused())
        e.ctx_focused()->camera.handleWorldNavigation(e, single_touch_pan);
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

/// ==========================
/// ======== Viewport ========
/// ==========================

Viewport::Viewport(Layout* layout, Canvas* canvas, int viewport_index, int grid_x, int grid_y) :
    layout(layout),
    viewport_index(viewport_index),
    viewport_grid_x(grid_x),
    viewport_grid_y(grid_y)
{
    // Tell derived painter to paint to the same canvas
    setGlobalScale(canvas->getGlobalScale());
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

    /*translate(
        floor(camera.originPixelOffset().x + camera.panPixelOffset().x),
        floor(camera.originPixelOffset().y + camera.panPixelOffset().y)
    );

    rotate(camera.rotation);
    translate(
        floor(-camera.x * camera.zoomX()),
        floor(-camera.y * camera.zoomY())
    );

    scale(camera.zoomX(), camera.zoomY());*/

    //camera.updateCameraMatrix();

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
        fillText(line, 5, 5 + (line_index++ * getGlobalScale() * 16.0f));

    camera.restoreCameraTransform();
    restore();
}

void Viewport::printTouchInfo()
{
    double pinch_zoom_factor = camera.touchDist() / camera.panDownTouchDist();

    print() << "cam.position:               (" << camera.x() << ",  " << camera.y() << ")\n";
    print() << "cam.pan:                     (" << camera.panX() << ",  " << camera.panY() << ")\n";
    print() << "cam.rotation:              " << camera.rotation() << "\n";
    print() << "cam.zoom:                  (" << camera.zoomX() << ", " << camera.zoomY() << ")\n\n";
    print() << "---------------------------------------------------------------\n";
    print() << "pan_down_touch_x           = " << camera.panDownTouchX() << "\n";
    print() << "pan_down_touch_y           = " << camera.panDownTouchY() << "\n";
    print() << "pan_down_touch_dist      = " << camera.panDownTouchDist() << "\n";
    print() << "pan_down_touch_angle   = " << camera.panDownTouchAngle() << "\n";
    print() << "---------------------------------------------------------------\n";
    print() << "pan_beg_cam_x                = " << camera.panBegCamX() << "\n";
    print() << "pan_beg_cam_y                = " << camera.panBegCamY() << "\n";
    print() << "pan_beg_cam_zoom_x    = " << camera.panBegCamZoomX() << "\n";
    print() << "pan_beg_cam_zoom_y    = " << camera.panBegCamZoomY() << "\n";
    print() << "pan_beg_cam_angle        = " << camera.panBegCamAngle() << "\n";
    print() << "---------------------------------------------------------------\n";
    print() << "cam.touchAngle()        = " << camera.touchAngle() << "\n";
    print() << "cam.touchDist()         = " << camera.touchDist() << "\n";
    print() << "---------------------------------------------------------------\n";
    print() << "pinch_zoom_factor   = " << pinch_zoom_factor << "x\n";
    print() << "---------------------------------------------------------------\n";

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

/// =========================
/// ======== Project ========
/// =========================

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

        ImGui::Dummy(ScaleSize(0, 8));
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
    DebugPrint("_projectPrepare()");
    logMessage("_projectPrepare()");
    viewports.clear();

    //ImGui::updatePointerValues();

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
    DVec2 surface_size = surfaceSize();

    // Update viewport sizes
    double viewport_width = surface_size.x / viewports.cols;
    double viewport_height = surface_size.y / viewports.rows;

    // Update viewport rects
    for (Viewport* viewport : viewports)
    {
        viewport->pos_x = viewport->viewport_grid_x * viewport_width;
        viewport->pos_y = viewport->viewport_grid_y * viewport_height;
        viewport->width = viewport_width - 1;
        viewport->height = viewport_height - 1;
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
            double viewport_mx = mouse.client_x - viewport->pos_x;
            double viewport_my = mouse.client_y - viewport->pos_y;

            Camera& cam = viewport->camera;

            if (cam.panning ||
                (viewport_mx >= 0 && viewport_my >= 0 &&
                 viewport_mx <= viewport->width && viewport_my <= viewport->height))
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
            //cam.updateCameraMatrix();
        }

        if (!paused)
        {
            auto project_t0 = std::chrono::steady_clock::now();

            // Process each scene
            for (SceneBase* scene : viewports.all_scenes)
            {
                scene->dt_call_index = 0;
                auto scene_t0 = std::chrono::steady_clock::now();

                //scene->_markLiveValues();

                scene->mouse = &mouse;
                scene->sceneProcess();

                auto scene_dt = std::chrono::steady_clock::now() - scene_t0;
                scene->dt_sceneProcess = static_cast<int>(
                    std::chrono::duration_cast<std::chrono::milliseconds>(scene_dt).count()
                );

                //scene->updateSceneShadowBuffer();
            }


            // Allow project to handle process on each Viewport
            for (Viewport* viewport : viewports)
            {
                //viewport->camera.panZoomProcess();
                viewport->scene->camera = &viewport->camera;
                Camera::active = &viewport->camera;

                ///viewport->just_resized =
                ///    (viewport->width != viewport->old_width) ||
                ///    (viewport->height != viewport->old_height);

                viewport->scene->viewportProcess(viewport, dt_frameProcess/1000.0);

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

    ctx->setFontSize(16.0f);

    ///timer_projectDraw.start();

    // Draw each viewport
    for (Viewport* viewport : viewports)
    {
        ctx->setClipRect(viewport->pos_x, viewport->pos_y, viewport->width, viewport->height);
        ctx->save();

        /// ======== Set default viewport transform ========

        // Move to viewport position
        ctx->translate(floor(viewport->pos_x), floor(viewport->pos_y));

        /// ======== Save default transform & Render ========

        // Set default transform to "World"
        viewport->camera.worldTransform();

        // Draw Scene to Viewport
        Camera::active = &viewport->camera;
        viewport->scene->camera = &viewport->camera;

        viewport->draw();

        /// ======== Restore initial canvas transform ========

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
            double line_x = floor(viewport->pos_x + viewport->width) + 0.5;
            ctx->moveTo(line_x, viewport->pos_y);
            ctx->lineTo(line_x, viewport->pos_y + viewport->height + 1);
        }

        // Draw horiz line
        if (viewport->viewport_grid_y < viewports.rows - 1)
        {
            double line_y = floor(viewport->pos_y + viewport->height) + 0.5;
            ctx->moveTo(viewport->pos_x + viewport->width + 1, line_y);
            ctx->lineTo(viewport->pos_x, line_y);
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
        case SDL_EVENT_MOUSE_MOTION:
        {
            mouse.client_x = e.motion.x;
            mouse.client_y = e.motion.y;
        }
        break;

        case SDL_EVENT_FINGER_MOTION:
        {
            mouse.client_x = (double)(e.tfinger.x * (float)fboWidth());
            mouse.client_y = (double)(e.tfinger.y * (float)fboHeight());
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
        case SDL_EVENT_MOUSE_BUTTON_DOWN:
        case SDL_EVENT_FINGER_DOWN:
        {
            bool captured = false;
            for (Viewport* ctx : viewports)
            {
                if (ctx->viewportRect().hitTest(mouse.client_x, mouse.client_y))
                {
                    ctx_focused = ctx;
                    captured = true;
                    break;
                }
            }
            if (!captured)
                ctx_focused = nullptr;
        }
        break;

        case SDL_EVENT_MOUSE_MOTION:
        case SDL_EVENT_FINGER_MOTION:
        {
            bool captured = false;
            for (Viewport* ctx : viewports)
            {
                if (ctx->viewportRect().hitTest(mouse.client_x, mouse.client_y))
                {
                    ctx_hovered = ctx;
                    captured = true;
                    break;
                }
            }
            if (!captured)
                ctx_hovered = nullptr;
        }
        break;
    }


    // Track fingers
    if (Platform()->is_mobile())
    {
        // Does this finger already have an owner? Attach it to the event
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

            // Remember mouse-down ctx for mouse release/motion
            event.setOwnerViewport(ctx_owner);
        }
        break;

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
        }
        break;

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
        }
        break;
        }
    }
    else
    {
        switch (e.type)
        {
            case SDL_EVENT_MOUSE_BUTTON_DOWN:
            case SDL_EVENT_MOUSE_BUTTON_UP:
            case SDL_EVENT_MOUSE_MOTION:
            {
                event.setOwnerViewport(ctx_focused);
            }
            break;
            case SDL_EVENT_MOUSE_WHEEL:
            {
                event.setOwnerViewport(ctx_hovered);
            }
            break;
        }
    }


    event.setFocusedViewport(ctx_focused);
    event.setHoveredViewport(ctx_hovered);

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

void SceneBase::logMessage(std::string_view fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    project->project_log->vlog(fmt.data(), ap);
    va_end(ap);
}

void ProjectBase::pullDataFromShadow()
{
    ProjectWorker::instance()->pullDataFromShadow();
}

void ProjectBase::pollEvents()
{
    ProjectWorker::instance()->pollEvents(false);
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
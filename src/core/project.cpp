#include <bitloop/core/project.h>

#include <bitloop/core/project_worker.h>
#include <bitloop/core/main_window.h>
#include <bitloop/core/viewport.h>
#include <bitloop/core/scene.h>

BL_BEGIN_NS

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


void ProjectBase::configure(int _sim_uid, Canvas* _canvas, ImDebugLog* shared_log)
{
    blPrint() << "Project::configure";

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

        ImGui::Dummy(scale_size(0, 8));
    }
	
    // Populate UI for focused Scene
    if (ctx_focused)
    {
        SceneBase* scene = ctx_focused->scene;

        std::string sceneName = scene->name() + " " + std::to_string(scene->sceneIndex());
        std::string section_id = sceneName + "_section";

        bool showSceneUI = (viewports.count() == 1) || ImGui::CollapsingHeader(sceneName.c_str(), ImGuiTreeNodeFlags_DefaultOpen);

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

void ProjectBase::_populateOverlay()
{
    for (SceneBase* scene : viewports.all_scenes)
        scene->_populateOverlay();
}

void ProjectBase::updateLiveBuffers()
{
    for (SceneBase* scene : viewports.all_scenes)
        scene->updateLiveBuffers();
}
void ProjectBase::updateShadowBuffers()
{
    for (SceneBase* scene : viewports.all_scenes)
        scene->updateShadowBuffers();
}
bool ProjectBase::changedLive()
{
    for (SceneBase* scene : viewports.all_scenes) {
        if (scene->changedLive())
            return true;
    }
    return false;
}
bool ProjectBase::changedShadow()
{
    for (SceneBase* scene : viewports.all_scenes) {
        if (scene->changedShadow())
            return true;
    }
    return false;
}
void ProjectBase::markLiveValues()
{
    for (SceneBase* scene : viewports.all_scenes)
        scene->markLiveValues();
}
void ProjectBase::markShadowValues()
{
    for (SceneBase* scene : viewports.all_scenes)
        scene->markShadowValues();
}

void ProjectBase::updateUnchangedShadowVars()
{
    for (SceneBase* scene : viewports.all_scenes)
        scene->updateUnchangedShadowVars();
}

void ProjectBase::invokeScheduledCalls()
{
    for (SceneBase* scene : viewports.all_scenes)
        scene->invokeScheduledCalls();
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
    // Starting a project that's already been started/paused shouldn't occur
    assert(!paused);
    assert(!started);

    done_single_process = false;

    // Prepare layout
    _projectPrepare();

    /// Determine real viewport size, and cache starting size
    /// for future "scale_adjust" calculations
    updateViewportRects();

    for (Viewport* viewport : viewports)
        viewport->setInitialSize();

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
        viewport->scene->sceneMounted(viewport);
    }

    started = true;
}

void ProjectBase::_projectResume()
{
    if (paused)
    {
        // If paused - just resume, don't restart
        paused = false;
        return;
    }
}

void ProjectBase::_projectStop()
{
    projectStop();
    ///cache.finalize();

    ///shaders_loaded = false;
    started = false;
    paused = false; // Ensure project can start properly next 'play' (so it doesn't try to simple 'resume')
}

void ProjectBase::_projectPause()
{
    paused = true;
}

void ProjectBase::_projectDestroy()
{
    projectDestroy();
}

void ProjectBase::updateViewportRects()
{
    assert(viewports.count() > 0);

    DVec2 surface_size = canvas->fboSize();

    double visible_client_w = surface_size.x - ((viewports.cols-1) * splitter_thickness);
    double visible_client_h = surface_size.y - ((viewports.rows-1) * splitter_thickness);

    // Update viewport sizes
    double viewport_width  = (visible_client_w / viewports.cols);
    double viewport_height = (visible_client_h / viewports.rows);

    // Update viewport rects
    for (Viewport* viewport : viewports)
    {
        viewport->setSurfacePos(
            floor(viewport->viewport_grid_x * (viewport_width + splitter_thickness)),
            floor(viewport->viewport_grid_y * (viewport_height + splitter_thickness))
        );

        viewport->setSurfaceSize(
            ceil(viewport_width),
            ceil(viewport_height)
        );
    }
}

void ProjectBase::_projectProcess()
{
    auto frame_dt = std::chrono::steady_clock::now() - last_frame_time;
    dt_frameProcess = static_cast<double>(std::chrono::duration_cast<std::chrono::milliseconds>(frame_dt).count());
    
    last_frame_time = std::chrono::steady_clock::now();

    updateViewportRects();

    

    // 'did_first_process' is only true for the FIRST frame we process (when unpaused) so that we
    // don't do any drawing until this sets:  done_single_process=true
    // (this is to prevent recording the anything until the first real frame completes after starting record)
    bool did_first_process = false;

    // Allow panning/zooming, even when paused
    for (Viewport* viewport : viewports)
    {
        double viewport_mx = mouse.client_x - viewport->left();
        double viewport_my = mouse.client_y - viewport->top();

        auto m = viewport->inverseTransform<f128>();

        if (//cam.panning ||
            (viewport_mx >= 0 &&
                viewport_my >= 0 &&
                viewport_mx <= viewport->width() &&
                viewport_my <= viewport->height()))
        {
            mouse.viewport = viewport;
            mouse.stage_x = viewport_mx;
            mouse.stage_y = viewport_my;

            auto world_mouse = static_cast<DDVec2>(m * glm::ddvec3(viewport_mx, viewport_my, 1.0));
            mouse.world_x = world_mouse.x;
            mouse.world_y = world_mouse.y;
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

            //viewport->just_resized =
            //    (viewport->w != viewport->old_w) ||
            //    (viewport->h != viewport->old_h);

            double dt;
            if (main_window()->getRecordManager()->isRecording() )
                dt = 1.0 / main_window()->getRecordManager()->fps();
            else
                dt = dt_frameProcess / 1000.0;

            viewport->scene->viewportProcess(viewport, dt);

            //viewport->old_w = viewport->w;
            //viewport->old_h = viewport->h;
        }

        /// --- Post-Process each scene ---

        // Update 'live' values of all ChangeTracker variables (to compare against next frame)
        for (SceneBase* scene : viewports.all_scenes)
            scene->ChangeTracker::updateCurrent();

        // Measure time taken to process whole project
        auto project_dt = std::chrono::steady_clock::now() - project_t0;
        dt_projectProcess = static_cast<int>(
            std::chrono::duration_cast<std::chrono::milliseconds>(project_dt).count()
        );

        // Prepare to encode the next frame
        ///encode_next_paint = true;
        if (!done_single_process)
            did_first_process = true;
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
    DVec2 surface_size = canvas->fboSize();

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

        viewport->resetTransform();

        canvas->setClipRect(viewport->left(), viewport->top(), viewport->width(), viewport->height());
        canvas->save();

        // Starting viewport position
        canvas->translate(std::floor(viewport->left()), std::floor(viewport->top()));

        // Set default world transform
        viewport->worldMode();

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
        canvas->setLineWidth(splitter_thickness);
        canvas->setStrokeStyle(30, 30, 40);
        canvas->beginPath();

        // Draw vert line
        if (viewport->viewport_grid_x < viewports.cols - 1)
        {
            double line_x = viewport->right() + splitter_thickness / 2;
            canvas->moveTo(line_x, viewport->top());
            canvas->lineTo(line_x, viewport->bottom() + splitter_thickness); // +splitter_thickness to fill sqare gap between lines
        }

        // Draw horiz line
        if (viewport->viewport_grid_y < viewports.rows - 1)
        {
            double line_y = viewport->bottom() + splitter_thickness / 2;
            canvas->moveTo(viewport->left(), line_y);
            canvas->lineTo(viewport->right(), line_y);
        }

        canvas->stroke();

        if (ctx_focused == viewport)
        {
            canvas->setLineWidth(1);
            canvas->setStrokeStyle(75, 75, 100);
            canvas->strokeRect(viewport->left() - 0.5, viewport->top() - 0.5, viewport->width() + 1, viewport->height() + 1);
        }
    }

    for (Viewport* viewport : viewports)
        viewport->setOldSize();
}

void ProjectBase::_clearEventQueue()
{
    //for (SceneBase* scene : viewports.all_scenes)
    //{
    //    scene->input.clear();
    //}
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

    //TouchInput s;

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
            mouse.client_x = (double)(e.tfinger.x * (float)canvas->fboWidth());
            mouse.client_y = (double)(e.tfinger.y * (float)canvas->fboHeight());
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

    if (!ctx_focused && event.isPointerEvent())
        return; // If no "focused" viewport for mouse event, project has nothing to handle (likely imgui)

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

                // Remember touched-down ctx for touch release/motion event to use same owner
                event.setOwnerViewport(ctx_owner);
                PointerEvent pointer_event(event);

                // Add pressed finger
                FingerInfo info;
                info.ctx_owner = ctx_focused;
                info.fingerId = e.tfinger.fingerID;
                info.x = pointer_event.x();
                info.y = pointer_event.y();
                pressed_fingers.push_back(info);

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

                // Remember touched-down ctx for touch release/motion event to use same owner
                event.setOwnerViewport(ctx_owner);
                PointerEvent pointer_event(event);

                // Add pressed finger
                FingerInfo info;
                info.ctx_owner = ctx_focused;
                info.fingerId = 0;
                info.x = pointer_event.x();
                info.y = pointer_event.y();
                pressed_fingers.push_back(info);

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

    //if (event.ctx_owner())
    //{
    //    event.ctx_owner()->scene->input.addEvent(event, pressed_fingers);
    //}

    onEvent(event);

    for (SceneBase* scene : viewports.all_scenes)
    {
        // Keep specialized handlers onMouseDown/onKeyDown(etc) pre-filtered, 
        // but keep onEvent generic so users can retain full control.
        scene->_onEvent(event);
    }
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

void ProjectBase::logMessage(const char* fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    project_log->vlog(fmt, ap);
    va_end(ap);
}

void ProjectBase::logClear()
{
    project_log->clear();
}

BL_END_NS

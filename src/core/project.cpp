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

ProjectBase* ProjectBase::activeProject()
{
    return project_worker()->current_project;
}


void ProjectBase::configure(int _sim_uid, NanoCanvas* _canvas, ImDebugLog* shared_log)
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
    if (getInterfaceModel() &&
        getInterfaceModel()->sidebarVisible())
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

        if (scene->getInterfaceModel() &&
            scene->getInterfaceModel()->sidebarVisible())
        {
            std::string sceneName = scene->name() + " " + std::to_string(scene->sceneIndex());
            std::string section_id = sceneName + "_section";

            // If we have only one viewport, always show its UI (and don't show collapsing header)
            bool showSceneUI =
                (viewports.count() == 1) ||
                ImGui::CollapsingHeader(sceneName.c_str(), ImGuiTreeNodeFlags_DefaultOpen);

            if (showSceneUI)
            {
                // Allow Scene to populate inputs for section
                ImGui::PushID(section_id.c_str());

                // blue
                ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.3f, 0.35f, 0.60f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.35f, 0.42f, 0.7f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0.2f, 0.5f, 0.8f, 1.0f));

                // red
                //ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.5f, 0.2f, 0.2f, 1.0f));
                //ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.6f, 0.3f, 0.3f, 1.0f));
                //ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(1.0f, 0.4f, 0.4f, 1.0f));

                scene->_sceneAttributes();

                ImGui::PopStyleColor(3);
                ImGui::PopID();
            }
        }
    }
}

bool DrawFullscreenOverlayButton(bool* fullscreen, bool* was_held, ImVec2 screen_pos, float size = 30.0f, float alpha = 0.66f)
{
    ImDrawList* dl = ImGui::GetForegroundDrawList();
    ImGuiIO& io = ImGui::GetIO();

    const ImRect r(screen_pos, screen_pos + ImVec2(size, size));

    // Colors
    const ImU32 col_bg = ImGui::GetColorU32(ImVec4(0, 0, 0, alpha));
    const ImU32 col_bg_h = ImGui::GetColorU32(ImVec4(0, 0, 0, alpha + 0.15f));
    const ImU32 col_bg_p = ImGui::GetColorU32(ImVec4(0, 0, 0, alpha + 0.25f));
    const ImU32 col_bd = ImGui::GetColorU32(ImVec4(1, 1, 1, 0.25f));
    const ImU32 col_fg = ImGui::GetColorU32(ImGui::GetStyle().Colors[ImGuiCol_Text]);

    // Hit test
    const bool hovered = r.Contains(io.MousePos);
    const bool held = hovered && ImGui::IsMouseDown(ImGuiMouseButton_Left);
    const bool clicked = hovered && *was_held && ImGui::IsMouseReleased(ImGuiMouseButton_Left);
    *was_held = held;

    // Background
    const float rounding = size / 6.0f;
    dl->AddRectFilled(r.Min, r.Max, held ? col_bg_p : (hovered ? col_bg_h : col_bg), rounding);
    dl->AddRect(r.Min, r.Max, col_bd, rounding);

    // Icon geometry
    const float t = scale_size(2.0f);     // stroke thickness
    const float inset = size * 0.08f;         // distance from edges
    const float len = size * 0.15f;         // target leg length

    const float max_len = ImMax(0.0f, (size - 2.0f * inset) * 0.5f);
    const float L = ImMin(len, max_len);

    auto lineL = [&](const ImVec2& c, const ImVec2& dx, const ImVec2& dy)
    {
        dl->AddLine(c, c + dx * L, col_fg, t);
        dl->AddLine(c, c + dy * L, col_fg, t);
    };

    // Outer inset corners
    const ImVec2 tl = r.Min + ImVec2(inset, inset);
    const ImVec2 tr = ImVec2(r.Max.x - inset, r.Min.y + inset);
    const ImVec2 bl = ImVec2(r.Min.x + inset, r.Max.y - inset);
    const ImVec2 br = r.Max - ImVec2(inset, inset);

    if (!*fullscreen)
    {
        // Expand: inner box corners (open outward)
        const ImVec2 ia = r.Min + ImVec2(inset + L, inset + L);
        const ImVec2 ib = r.Max - ImVec2(inset + L, inset + L);

        lineL(ImVec2(ia.x, ia.y), ImVec2(+1, 0), ImVec2(0, +1)); // top-left
        lineL(ImVec2(ib.x, ia.y), ImVec2(-1, 0), ImVec2(0, +1)); // top-right
        lineL(ImVec2(ia.x, ib.y), ImVec2(+1, 0), ImVec2(0, -1)); // bottom-left
        lineL(ImVec2(ib.x, ib.y), ImVec2(-1, 0), ImVec2(0, -1)); // bottom-right
    }
    else
    {
        // Collapse: from outer corners inward
        const float inset2 = inset * 2.0f;
        const ImVec2 ia = r.Min + ImVec2(inset2 + L, inset2 + L);
        const ImVec2 ib = r.Max - ImVec2(inset2 + L, inset2 + L);

        lineL(ImVec2(ia.x, ia.y), ImVec2(-1, 0), ImVec2(0, -1)); // top-left inward
        lineL(ImVec2(ib.x, ia.y), ImVec2(+1, 0), ImVec2(0, -1)); // top-right inward
        lineL(ImVec2(ia.x, ib.y), ImVec2(-1, 0), ImVec2(0, +1)); // bottom-left inward
        lineL(ImVec2(ib.x, ib.y), ImVec2(+1, 0), ImVec2(0, +1)); // bottom-right inward
    }

    if (clicked)
    {
        *fullscreen = !*fullscreen;
        return true;
    }
    return false;
}

inline bool DrawSidebarOverlayButton(
    bool* sidebar_visible,
    ImVec2 screen_pos,
    float btn_w = 22.0f,
    float btn_h = 30.0f,
    float alpha = 0.66f)
{
    if (!sidebar_visible)
        return false;

    ImDrawList* dl = ImGui::GetForegroundDrawList();
    ImGuiIO& io = ImGui::GetIO();

    const ImVec2 rmin = screen_pos;                    // top-left
    const ImVec2 rmax = screen_pos + ImVec2(btn_w, btn_h);

    const ImU32 col_bg = ImGui::GetColorU32(ImVec4(0, 0, 0, alpha));
    const ImU32 col_bg_h = ImGui::GetColorU32(ImVec4(0, 0, 0, ImClamp(alpha + 0.15f, 0.0f, 1.0f)));
    const ImU32 col_bg_p = ImGui::GetColorU32(ImVec4(0, 0, 0, ImClamp(alpha + 0.25f, 0.0f, 1.0f)));
    const ImU32 col_bd = ImGui::GetColorU32(ImVec4(1, 1, 1, 0.25f));
    const ImU32 col_fg = ImGui::GetColorU32(ImGui::GetStyle().Colors[ImGuiCol_Text]);

    const bool hovered = (io.MousePos.x >= rmin.x && io.MousePos.x < rmax.x &&
        io.MousePos.y >= rmin.y && io.MousePos.y < rmax.y);
    const bool held = hovered && ImGui::IsMouseDown(ImGuiMouseButton_Left);

    ImGui::PushID(sidebar_visible);
    const ImGuiID hold_id = ImGui::GetID("##sidebar_overlay_hold");
    ImGui::PopID();

    ImGuiStorage* st = ImGui::GetStateStorage();
    const bool was_held = st->GetBool(hold_id, false);
    const bool clicked = hovered && was_held && ImGui::IsMouseReleased(ImGuiMouseButton_Left);
    st->SetBool(hold_id, held);

    const float rounding = btn_h / 6.0f;
    const ImDrawFlags round_left =
        ImDrawFlags_RoundCornersTopLeft | ImDrawFlags_RoundCornersBottomLeft;

    dl->AddRectFilled(rmin, rmax, held ? col_bg_p : (hovered ? col_bg_h : col_bg), rounding, round_left);
    dl->AddRect(rmin, rmax, col_bd, rounding, round_left);

    const ImVec2 c((rmin.x + rmax.x) * 0.5f, (rmin.y + rmax.y) * 0.5f);
    const float t = scale_size(2.0f);

    const float chevron_w = btn_w * 0.20f;
    const float chevron_h = btn_h * 0.13f;

    if (*sidebar_visible)
    {
        const ImVec2 p0(c.x - chevron_w, c.y - chevron_h);
        const ImVec2 p1(c.x + chevron_w, c.y);
        const ImVec2 p2(c.x - chevron_w, c.y + chevron_h);
        dl->AddLine(p0, p1, col_fg, t);
        dl->AddLine(p2, p1, col_fg, t);
    }
    else
    {
        const ImVec2 p0(c.x + chevron_w, c.y - chevron_h);
        const ImVec2 p1(c.x - chevron_w, c.y);
        const ImVec2 p2(c.x + chevron_w, c.y + chevron_h);
        dl->AddLine(p0, p1, col_fg, t);
        dl->AddLine(p2, p1, col_fg, t);
    }

    if (clicked)
    {
        *sidebar_visible = !*sidebar_visible;
        return true;
    }
    return false;
}

void ProjectBase::_populateOverlay()
{
    if (show_fullscreen_btn)
    {
        IVec2 ctx_size = main_window()->viewportSize();
        float btn_space = scale_size(6.0f);
        float fullscreen_btn_size = scale_size(56.0f);
        float sidebar_btn_size = scale_size(22.0f);

        // fullscreen button
        SDL_WindowFlags flags = SDL_GetWindowFlags(platform()->sdl_window());
        bool fullscreen = (flags & SDL_WINDOW_FULLSCREEN);
        static bool fullscreen_btn_held = false;
        if (DrawFullscreenOverlayButton(&fullscreen, &fullscreen_btn_held,
            ImVec2(ctx_size.x - fullscreen_btn_size - sidebar_btn_size - btn_space, btn_space), fullscreen_btn_size))
        {
            SDL_SetWindowFullscreen(platform()->sdl_window(), fullscreen);
            main_window()->setSidebarVisible(!fullscreen);
        }

        // show/hide sidebar button
        bool sidebar_visible = main_window()->getSidebarVisible();
        if (DrawSidebarOverlayButton(&sidebar_visible, ImVec2(ctx_size.x - sidebar_btn_size, btn_space), sidebar_btn_size, fullscreen_btn_size))
        {
            main_window()->setSidebarVisible(sidebar_visible);
        }
    }

    for (SceneBase* scene : viewports.all_scenes)
        scene->_populateOverlay();
}

void ProjectBase::_initGUI()
{
    for (SceneBase* scene : viewports.all_scenes)
        scene->_initGUI();
}
void ProjectBase::_destroyGUI()
{
    for (SceneBase* scene : viewports.all_scenes)
        scene->_destroyGUI();
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
void ProjectBase::updateUnchangedShadowVars()
{
    for (SceneBase* scene : viewports.all_scenes)
        scene->updateUnchangedShadowVars();
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
void ProjectBase::invokeScheduledCalls()
{
    for (SceneBase* scene : viewports.all_scenes)
        scene->invokeScheduledCalls();
}

void ProjectBase::_projectPrepare()
{
    if (started)
        viewports.clear();
    else
        assert(viewports.count() == 0);

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
    // TODO: prepare if not yet prepared?
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

        scene->_sceneStart();
    }

    main_window()->threadQueue().invokeBlocking([&]()
    {
        _initGUI();
        main_window()->threadQueue().drain();
    });

    // Mount to viewports
    for (Viewport* viewport : viewports)
    {
        viewport->scene->sceneMounted(viewport);
        viewport->scene->requestRedraw(true);
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
    if (!started)
        return; // nothing to destroy

    for (SceneBase* scene : viewports.all_scenes)
        scene->_sceneDestroy();

    projectDestroy();

    // force any UI / deferred destructions to happen on the main GUI thread *before* deleting the project/scenes
    // destroy scene->ui's, but not project->ui (since it can still be reconfigered and start again)
    main_window()->threadQueue().invokeBlocking([&]()
    {
        for (SceneBase* scene : viewports.all_scenes)
            scene->_destroyGUI();

        main_window()->threadQueue().drain();
    });

    // deletes Scene's *after* Scene::_destroyGUI 
    viewports.clear();
}

bool ProjectBase::updateViewportRects()
{
    assert(viewports.count() > 0);

    bool resized = false;

    DRect client_rect = (DRect)canvas->clientRect();

    DVec2 surface_size = canvas->fboSize();
    DVec2 client_size = client_rect.size();

    if (surface_size != old_surface_size)
    {
        old_surface_size = surface_size;
        resized = true;
    }

    // stretch factor from surface_size to rendered client_size
    DVec2 sf = client_size / surface_size; 

    double visible_client_w = surface_size.x - ((viewports.cols-1) * splitter_thickness);
    double visible_client_h = surface_size.y - ((viewports.rows-1) * splitter_thickness);

    // Update viewport sizes
    double viewport_width  = (visible_client_w / viewports.cols);
    double viewport_height = (visible_client_h / viewports.rows);

    // Update viewport rects
    for (Viewport* viewport : viewports)
    {
        // todo: resizing splitter? Only makes sense if DPR hasn't changed
        viewport->setSurfacePos(
            floor(viewport->viewport_grid_x * (viewport_width + splitter_thickness)),
            floor(viewport->viewport_grid_y * (viewport_height + splitter_thickness))
        );

        viewport->setSurfaceSize(
            ceil(viewport_width),
            ceil(viewport_height)
        );

        viewport->setClientRect(
            client_rect.x1 + (viewport->left() * sf.x),
            client_rect.y1 + (viewport->top() * sf.y),
            viewport->width() * sf.x,
            viewport->height() * sf.y
        );
    }

    return resized;
}

void ProjectBase::_projectProcess()
{
    frame_dt = frame_timer.tick();

    bool canvas_dirty = false;
    bool viewport_rects_updated = updateViewportRects();

    // 'did_first_process' is only true for the FIRST frame we process (when unpaused) so that we
    // don't do any drawing until this sets:  done_single_process=true
    // (this is to prevent recording the anything until the first real frame completes after starting record)
    bool did_first_process = false;

    // Allow panning/zooming, even when paused
    for (Viewport* viewport : viewports)
    {
        double viewport_mx = viewport->toSurfaceX(mouse.client_x);
        double viewport_my = viewport->toSurfaceY(mouse.client_y);
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
        project_timer.begin();

        // Process each scene
        for (SceneBase* scene : viewports.all_scenes)
        {
            scene_timer.begin();

            ///scene->needs_redraw = true; // force redraw unless told otherwise
            scene->mouse = &mouse;
            scene->sceneProcess();

            scene->dt_sceneProcess = scene_timer.elapsed();
        }


        // Allow project to handle process on each Viewport
        for (Viewport* viewport : viewports)
        {
            //viewport->camera.panZoomProcess();

            double dt;
            if (main_window()->getCaptureManager()->isRecording() )
                dt = 1.0 / main_window()->getCaptureManager()->fps();
            else
                dt = frame_dt / 1000.0;

            viewport->scene->viewportProcess(viewport, dt);

            // all viewports need redraw if surface changes size
            if (viewport_rects_updated)
                viewport->scene->needs_redraw = true;

            // if any scene needs a redraw (including from surface size change), mark the canvas
            // as dirty so it gets redrawn by imgui
            if (viewport->scene->needs_redraw)
                canvas_dirty = true;
        }

        /// --- Post-Process each scene ---

        // Measure time taken to process whole project
        project_dt = project_timer.elapsed();

        // Prepare to encode the next frame
        ///encode_next_paint = true;
        if (!done_single_process)
            did_first_process = true;
    }

    if (did_first_process)
        done_single_process = true;

    if (canvas_dirty)
        canvas->setDirty();
}
void ProjectBase::_projectDraw()
{
    // note: called from main GUI thread

    if (!done_single_process) return;
    if (!started) return;

    DVec2 surface_size = canvas->fboSize();

    // todo: No per-viewport redrawing for now. Would require:
    // - or give each viewport a separate Canvas (ideal)
    //   - let ImGui draw viewport splitters
    //   - blit each viewport (when dirty) to merged "project_image" so you can record all in one frame
    //   - optional [hard]: add Scene-specific logic to capture only that scene.
    //     - queue captures, see if FFmpeg can handle multiple recordings at once (multiple capture managers?)
    // 
    // - additional surfaces to target (switch same nanovg instance)
    
    // If any viewport is dirty, redraw all viewports
    bool any_viewports_dirty = false;
    for (Viewport* viewport : viewports)
    {
        if (viewport->scene->needs_redraw || viewport->focused_dt > 0)
        {
            any_viewports_dirty = true;
            viewport->scene->needs_redraw = false;
        }
    }

    // Draw background
    if (any_viewports_dirty)
    {
        canvas->setFillStyle(10, 10, 15);
        canvas->fillRect(0, 0, surface_size.x, surface_size.y);

        canvas->setFillStyle(255, 255, 255);
        canvas->setStrokeStyle(255, 255, 255);

        canvas->setFontSize(16.0f);

        ///timer_projectDraw.start();

        // Draw each viewport
        for (Viewport* viewport : viewports)
        {
            // Reuse derived painter for the primary canvas context
            viewport->setTargetPainterContext(canvas->getPainterContext());

            viewport->resetTransform();

            canvas->setClipRect(viewport->left(), viewport->top(), viewport->width(), viewport->height());
            canvas->save();

            // Starting viewport position
            canvas->translate(std::floor(viewport->left()), std::floor(viewport->top()));

            // Set default world transform
            viewport->worldMode();

            // Draw Scene to Viewport
            ///if (viewport->scene->needs_redraw)
            {
                viewport->draw();
                ///viewport->scene->needs_redraw = false;
            }

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
                int flash_extra = 0;
                if (viewport->focused_dt > 0)
                {
                    viewport->focused_dt -= 1.0f;

                    float focus_flash_brightness = viewport->focused_dt / Viewport::focus_flash_frames;
                    flash_extra = (int)(focus_flash_brightness * 155.0f);
                }

                canvas->setLineWidth(1);
                canvas->setStrokeStyle(75 + flash_extra, 75, 100 + flash_extra);
                canvas->strokeRect(viewport->left() - 0.5, viewport->top() - 0.5, viewport->width() + 1, viewport->height() + 1);
            }
        }
    }

    for (Viewport* viewport : viewports)
        viewport->setOldSize();
}
void ProjectBase::_onEndFrame()
{
    for (SceneBase* scene : viewports.all_scenes)
        scene->onEndFrame();

    // Update 'live' values of all ChangeTracker variables (to compare against next frame)
    for (SceneBase* scene : viewports.all_scenes)
        scene->ChangeTracker::updateCurrent();

    mouse.clearStaleButtonStates();
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

    /// TODO: Is "hovered" viewport really necessary? Stick with focused viewport?

    // Wrap SDL event
    Event event(e);

    // If we focus on a different viewport, ignore that touch event
    bool captured_focus_event = false;

    //TouchInput s;

    // Track "cursor" position
    switch (e.type)
    {
        // Mouse
        case SDL_EVENT_MOUSE_BUTTON_DOWN:
        {
            if (platform()->is_mobile()) return;
            SDL_MouseButtonEvent& b = e.button;

            mouse.buttonState((MouseButton)b.button).is_down = true;
            mouse.buttonState((MouseButton)b.button).clicked = true;

            mouse.client_x = e.button.x;
            mouse.client_y = e.button.y;
        } break;
        case SDL_EVENT_MOUSE_BUTTON_UP:
        {
            if (platform()->is_mobile()) return;
            SDL_MouseButtonEvent& b = e.button;

            mouse.buttonState((MouseButton)b.button).is_down = false;
            mouse.client_x = b.x;
            mouse.client_y = b.y;
        } break;
        case SDL_EVENT_MOUSE_MOTION:
        {
            if (platform()->is_mobile()) return;
            SDL_MouseButtonEvent& b = e.button;

            mouse.client_x = b.x;
            mouse.client_y = b.y;
        } break;

        // Touch
        case SDL_EVENT_FINGER_DOWN:
        case SDL_EVENT_FINGER_UP:
        case SDL_EVENT_FINGER_MOTION:
        {
            mouse.client_x = (double)(e.tfinger.x * platform()->fbo_width());
            mouse.client_y = (double)(e.tfinger.y * platform()->fbo_height());
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
                if (ctx->clientRect().hitTest(mouse.client_x, mouse.client_y))
                {
                    if (ctx != ctx_focused)
                    {
                        ctx_focused = ctx;
                        captured_focus_event = true;
                        ctx->focused_dt = Viewport::focus_flash_frames;
                    }
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
                if (ctx->clientRect().hitTest(mouse.client_x, mouse.client_y))
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

    if (captured_focus_event || (!ctx_focused && event.isPointerEvent()))
        return; // If switched focus, or there is no "focused" viewport for mouse event, project has nothing to handle (likely imgui)

    // Update pressed_fingers
    {
        // Does this finger already have an "owner" viewport?
        // If so, loop over fingers and restore it for 
        Viewport* ctx_owner = nullptr;

        // Support both single-finger pan & 2 finger transform
        switch (e.type)
        {
            // ----- MOBILE -----
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

            // ----- DESKTOP -----
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

            case SDL_EVENT_KEY_DOWN:
            case SDL_EVENT_KEY_UP:
            {
                ctx_owner = ctx_focused;
                event.setOwnerViewport(ctx_owner);
            }
            break;
        }
    }

    if (platform()->is_mobile())
    {
        // simulate left-mouse button on mobile
        bool any_pressed = pressed_fingers.size() > 0;
        mouse.buttonState(MouseButton::LEFT).is_down = any_pressed;
        mouse.buttonState(MouseButton::LEFT).clicked = any_fingers_pressed && !any_pressed;

        // store old state
        any_fingers_pressed = any_pressed;
    }
    
    event.setFocusedViewport(ctx_focused);
    event.setHoveredViewport(ctx_hovered);

    //if (event.ctx_owner())
    //{
    //    event.ctx_owner()->scene->input.addEvent(event, pressed_fingers);
    //}

    // enter/exit fullscreen hotkeys (F11 / Escape)
    if (event.type() == SDL_EVENT_KEY_DOWN)
    {
        if (event.sdl()->key.key == SDLK_F11)
        {
            SDL_WindowFlags flags = SDL_GetWindowFlags(platform()->sdl_window());
            bool fullscreen = (flags & SDL_WINDOW_FULLSCREEN);

            SDL_SetWindowFullscreen(platform()->sdl_window(), !fullscreen);
        }
        else if (event.sdl()->key.key == SDLK_ESCAPE)
        {
            SDL_SetWindowFullscreen(platform()->sdl_window(), false);
        }
    }

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

void ProjectBase::_onEncodeFrame(EncodeFrame& data, int request_id, const CapturePreset& preset)
{
    for (SceneBase* scene : viewports.all_scenes)
        scene->_onEncodeFrame(data, request_id, preset);
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

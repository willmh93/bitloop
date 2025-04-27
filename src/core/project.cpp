#include "project.h"


/// Scene

void Scene::registerMount(Viewport* viewport)
{
    //options = viewport->options;
    //camera = &viewport->camera;

    mounted_to_viewports.push_back(viewport);
    ///qDebug() << "Mounted to Viewport: " << viewport->viewportIndex();

    std::vector<Scene*>& all_scenes = viewport->layout->all_scenes;

    // Only add scene to list if it's not already in the layout (mounted to another viewport)
    if (std::find(all_scenes.begin(), all_scenes.end(), this) == all_scenes.end())
        viewport->layout->all_scenes.push_back(this);
}

void Scene::registerUnmount(Viewport* viewport)
{
    std::vector<Scene*>& all_scenes = viewport->layout->all_scenes;

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

void Scene::mountTo(Viewport* viewport)
{
    viewport->mountScene(this);
}

void Scene::mountTo(Layout& viewports)
{
    viewports[viewports.count()]->mountScene(this);
}

void Scene::mountToAll(Layout& viewports)
{
    for (Viewport* viewport : viewports)
        viewport->mountScene(this);
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
    setTextAlign(TextAlign::ALIGN_LEFT);
    setTextBaseline(TextBaseline::BASELINE_TOP);

    // Take snapshot of default transformation
    default_viewport_transform = currentTransform();

    // When resizing window, world coordinate is fixed given viewport anchor
    // If TOP_LEFT, the world coordinate at top left remains fixed
    // If CENTER, world coordinate at middle of viewport remains fixed

    /// QNanoPainter transformations
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

    // Draw mounted project to this viewport
    scene->camera = &camera;
    scene->viewportDraw(this);

    save();
    camera.saveCameraTransform();
    camera.stageTransform();
    setTextAlign(TextAlign::ALIGN_LEFT);
    setTextBaseline(TextBaseline::BASELINE_TOP);
    setFillStyle(255, 255, 255);

    ///auto lines = print_text.split('\n');
    ///for (int i = 0; i < lines.size(); i++)
    ///    fillText(lines[i], 5, 5 + (i * 16));

    camera.restoreCameraTransform();
    restore();
}

/// Layout

void Layout::add(int _viewport_index, int _grid_x, int _grid_y)
{
    Viewport* viewport = new Viewport(this, project->canvas, _viewport_index, _grid_x, _grid_y);
    viewports.push_back(viewport);
}

void Layout::resize(int viewport_count)
{
    if (targ_viewports_x <= 0 || targ_viewports_y <= 0)
    {
        // Spread proportionally
        rows = sqrt(viewport_count);
        cols = viewport_count / rows;
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

void Layout::expandCheck(size_t count)
{
    if (count > viewports.size())
        resize(count);
}

/// Project

void Project::configure(int _sim_uid, Canvas* _canvas)
{
    sim_uid = _sim_uid;
    canvas = _canvas;

    viewports.project = this;

    //options = _options;
    //options->setCurrentProject(this);

    started = false;
    paused = false;
}

void Project::_populateAttributes()
{
    //ImGui::SeparatorText("Project");
    if (ImGui::CollapsingHeader("Project", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::PushID("project_section");
        projectAttributes();
        ImGui::PopID();
    }

    ImGui::Dummy(ImVec2(0, 8));
    //ImGui::SeparatorText("Scenes");

    // Scene sections
    Layout& layout = currentLayout();
    auto& scenes = layout.scenes();
    for (Scene* scene : scenes)
    {
        std::string sceneName = scene->name() + " " + std::to_string(scene->sceneIndex());
        if (ImGui::CollapsingHeader(sceneName.c_str(), ImGuiTreeNodeFlags_DefaultOpen))
        {
            // Allow Scene to populate inputs for section
            ImGui::PushID((sceneName + "_section").c_str());
            scene->sceneAttributes();
            ImGui::PopID();
        }
    }
}

void Project::_projectPrepare()
{
    viewports.clear();


    //ImGui::updatePointerValues();

    // Prepare project and create layout
    // Note: This is where old viewports get replaced
    scene_counter = 0;
    projectPrepare();
}

void Project::_projectStart()
{
    if (paused)
    {
        // If paused - just resume, don't restart
        paused = false;
        return;
    }

    ///done_single_process = false;

    // Prepare layout
    _projectPrepare();

    // Update layout rects
    updateViewportRects();

    // Call Project::projectStart()
    projectStart();

    //cache.init("cache.bin");

    // Start project scenes
    for (Scene* scene : viewports.all_scenes)
    {
        ///scene->cache = &cache;
        scene->mouse = &mouse;
        ///scene->dt_scene_ma_list.clear();
        ///scene->dt_project_ma_list.clear();
        ///scene->dt_project_draw_ma_list.clear();
        ///scene->variableChangedClearMaps();

        scene->sceneStart();
    }

    // Mount to viewports
    for (Viewport* viewport : viewports)
    {
        viewport->scene->camera = &viewport->camera;
        viewport->scene->sceneMounted(viewport);
    }

    ///if (record_on_start)
    ///    startRecording();

    setProjectInfoState(ProjectInfo::ACTIVE);

    started = true;
}

void Project::_projectStop()
{
    projectStop();
    ///cache.finalize();

    ///if (record_manager.isRecording())
    ///    finalizeRecording();

    setProjectInfoState(ProjectInfo::INACTIVE);
    ///shaders_loaded = false;
    started = false;
}

void Project::_projectPause()
{
    paused = true;
}

void Project::_projectDestroy()
{
    setProjectInfoState(ProjectInfo::INACTIVE);
    projectDestroy();
}

void Project::updateViewportRects()
{
    Vec2 surface_size = surfaceSize();

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

Vec2 Project::surfaceSize()
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
    //    Size record_resolution = options->getRecordResolution();
    //
    //    w = record_resolution.x;
    //    h = record_resolution.y;
    //}

    return Vec2(w, h);
}

void Project::_projectProcess()
{
    updateViewportRects();

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

                Vec2 world_mouse = cam.toWorld(viewport_mx, viewport_my);
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
            ///timer_projectProcess.start();

            // Process each scene
            for (Scene* scene : viewports.all_scenes)
            {
                ///scene->dt_call_index = 0;
                ///scene->timer_sceneProcess.start();
                scene->mouse = &mouse;
                scene->sceneProcess();
                ///scene->dt_sceneProcess = scene->timer_sceneProcess.elapsed();
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
            ///for (Scene* scene : viewports.all_scenes)
            ///{
            ///    scene->variableChangedUpdateCurrent();
            ///}
            ///
            ///dt_projectProcess = timer_projectProcess.elapsed();

            // Prepare to encode the next frame
            ///encode_next_paint = true;
        }
    }

    ///done_single_process = true;
}

void Project::_projectDraw()
{
    ///if (!done_single_process) return;
    if (!started) return;

    ///if (!shaders_loaded)
    ///{
    ///    _loadShaders();
    ///    shaders_loaded = true;
    ///}

    Canvas* ctx = canvas;
    Vec2 surface_size = surfaceSize();

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



void Project::setProjectInfoState(ProjectInfo::State state)
{
    getProjectInfo()->state = state;
    //options->refreshTreeUI();
}

Layout& Project::newLayout()
{
    viewports.clear();
    return viewports;
}

Layout& Project::newLayout(int targ_viewports_x, int targ_viewports_y)
{
    ///viewports.options = options;

    viewports.clear();
    viewports.setSize(targ_viewports_x, targ_viewports_y);

    return viewports;
}


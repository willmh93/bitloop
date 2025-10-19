#include "Test1.h"

SIM_BEG;

using namespace bl;

/// =========================
/// ======== Project ========
/// =========================

void Test1_Project::UI::sidebar()
{
    bl_scoped(viewport_count);
    ImGui::SliderInt("Viewport Count", &viewport_count, 1, 8);
}

void Test1_Project::projectPrepare(Layout& layout)
{
    /// Create multiple instance of a single Scene, mount to separate viewports
    //layout << create<Test1_Scene>(viewport_count);
    layout << create<Test1_Scene>(1);

    /// Or create a single Scene instance and view on multiple Viewports
    //auto* scene = create<Test_Scene>();
    //for (int i = 0; i < viewport_count; ++i)
    //    layout << scene;
}

/// =========================
/// ========= Scene =========
/// =========================

/*void Test_Scene_Attributes::populate()
{
   
}*/

void Test1_Scene::UI::sidebar()
{
    bl_scoped(transform_coordinates);
    bl_scoped(scale_lines);
    bl_scoped(scale_sizes);
    bl_scoped(rotate_text);
    bl_scoped(camera);

    ImGui::Checkbox("Transform coordinates", &transform_coordinates); // updated in realtime
    ImGui::Checkbox("Scale Lines & Text", &scale_lines); // updated in realtime
    ImGui::Checkbox("Scale Sizes", &scale_sizes); // updated in realtime
    ImGui::Checkbox("Rotate Text", &rotate_text); // updated in realtime

    if (ImGui::Section("View", true))
    {
        // imgui camera controls
        camera.populateUI();
    }

    //static ImRect vr = { 0.0f, 0.8f, 0.8f, 0.0f };
    //ImSpline::SplineEditor("X/Y Spline", &sync(spline), &vr);
}

void Test1_Scene::sceneStart()
{
    // Initialize scene
    for (int i = 0; i < 50; i++)
    {
        particles.push_back({
            random(-200, 200),
            random(-200, 200),
            random(-2, 2),
            random(-2, 2)
        });
    }
}

void Test1_Scene::sceneMounted(Viewport* ctx)
{
    // Initialize viewport
    camera.setSurface(ctx);
    camera.setOriginViewportAnchor(Anchor::CENTER);

    navigator.setTarget(camera);
    navigator.setDirectCameraPanning(true);
}

void Test1_Scene::sceneDestroy()
{
    // Destroy scene
}

// Very naive prime checker
bool is_prime(unsigned long long n) {
    if (n < 2) return false;
    for (unsigned long long i = 2; i * i <= n; ++i) {
        if (n % i == 0) return false;
    }
    return true;
}

void Test1_Scene::sceneProcess()
{
    for (Particle& p : particles)
    {
        p.x += p.vx;
        p.y += p.vy;
        p.vx -= p.x * 0.0001;
        p.vy -= p.y * 0.0001;
    }
}

void Test1_Scene::viewportProcess(Viewport*, double)
{
    //if (ctx->viewportIndex() == 1)
    //{
    //    // Process viewport update
    //    camera->rotation = sin(seed) * M_PI_2;
    //    camera->setZoom(1.0 + sin(seed + M_PI_2) * 0.5);
    //    camera->x = cos(seed + M_PI * 0.75) * 300;
    //    camera->y = sin(seed + M_PI * 0.75) * 150;
    //
    //    //camera->rotation += 0.1 * M_PI / 180.0;
    //    camera->transform_coordinates = transform_coordinates;
    //    camera->scale_lines = scale_lines;
    //    camera->rotate_text = rotate_text;
    //}
    //else
    ///{
    ///    //camera->setPos(camera_x, camera_y);
    ///    camera->setZoomX(zoom_x * zoom_mult);
    ///    camera->setZoomY(zoom_y * zoom_mult);
    ///    camera->setRotation(camera_rotation);
    ///}

    ///obj.align_x = 0.5;
    ///obj.align_y = 0.5;

    //obj.rotation += 0.01;

    ball_pos.x = mouse->world_x;
    ball_pos.y = mouse->world_y;
}



void Test1_Scene::viewportDraw(Viewport* ctx) const
{
    ctx->setTransform(camera.getTransform());

    //camera->x += 1;
    //ctx->beginPath();
    //ctx->circle(0, 0, 5);
    //ctx->fill();

    //static auto font = NanoFont::create("/data/fonts/UbuntuMono.ttf");
    //ctx->setFont(font);
    //ctx->setFontSize(16);

    // Draw scene
    //ctx->scaleGraphics(scale_graphics);
    ctx->drawWorldAxis();// 0.3, 0.04, 0);
    /*ctx->setLineWidth(10);
    ctx->beginPath();
    ctx->moveTo(0, 0);
    ctx->lineTo(100, 0);
    ctx->lineTo(100, 100);
    ctx->stroke();
    ctx->strokeRect(0, 0, 200, 200);

    ctx->fillText("Hello!", 20, 20);

    ctx->beginStageTransform();
    ctx->fillText("HUD Text", 20, 20);
    ctx->endTransform();

    camera->worldTransform();*/

    //ctx->beginWorldTransform();


    ctx->worldCoordinates(transform_coordinates);
    ctx->scalingLines(scale_lines);
    ctx->scalingSizes(scale_sizes);
    ctx->rotatingText(rotate_text);

    ///obj.align = { -1, -1 };
    ///obj.setStageRect(100, 100, ctx->width - 200, ctx->height - 200);
    ///obj.setBitmapSize(obj.w, obj.h);

    ///if (obj.needsReshading(ctx))
    ///{
    ///    qDebug() << "Reshading.........";
    ///    obj.forEachWorldPixel(ctx, [this](int x, int y, double wx, double wy)
    ///    {
    ///        obj.setPixel(x, y, wx, wy, wy, 255);
    ///    });
    ///}
    ///ctx->drawSurface(obj);

    ctx->setFillStyle(255, 0, 255);
    ctx->beginPath();
    for (const Particle& p : particles)
    {
        ctx->circle(p.x, p.y, 0.5);
    }
    ctx->fill();

    
    /*ctx->setFillStyle(255, 255, 255);
    ctx->setLineWidth(5);

    DQuad q = static_cast<DQuad>(DRect(-50, -50, 50, 50));

    

    ctx->strokeQuad(q);
    ctx->strokeRect(-100, -100, 200, 200);
    ctx->strokeEllipse(0, 0, 200, 200);
    ctx->strokeEllipse(0, 0, 400, 100);
    //ctx->beginPath();
    //ctx->circle(100, 100, 5);
    //ctx->fill();

    //camera->worldHudMode();
    //camera->setStageOffset(50, 0);
    ctx->beginPath();
    DVec2 p = DVec2(100, 100) + Offset(50, 50);
    ctx->circle(p, 2);
    ctx->fill();
    ctx->fillText("Fixed pixel offset", p);

    ctx->setStrokeStyle(0, 255, 0);
    ctx->beginPath();
    ctx->moveTo(100, 100);
    ctx->lineTo(p);
    ctx->stroke();
    //camera->setStageOffset(0, 0);
    //ctx->endTransform();



    ctx->setTextAlign(TextAlign::ALIGN_LEFT);
    ctx->setTextBaseline(TextBaseline::BASELINE_TOP);

    camera->setTransformFilters(
        true,
        false,
        false,
        false
    );

    ///ctx->strokeQuad(obj.worldQuad());

    // Camera
    ctx->setFillStyle(255, 0, 0);
    ctx->beginPath();
    ctx->circle(camera->x(), camera->y(), 5);
    ctx->fill();

    ctx->fillText("Camera", camera->pos() + Offset(20, 20));*/

    /*camera->stageMode();

    std::stringstream txt;
    txt << "Size: ";
    int w, h;
    getDrawableSize(&w, &h);
    txt << w;
    txt << ", ";
    txt << h;
    ctx->fillText(txt.str(), 4, 4);

    txt = std::stringstream();
    txt << "DisplayFramebufferScale: ";
    ImGuiIO& io = ImGui::GetIO();
    txt << io.DisplayFramebufferScale.x;
    txt << ", ";
    txt << io.DisplayFramebufferScale.y;
    ctx->fillText(txt.str(), 4, 24);*/

    

    ctx->setFillStyle(0, 255, 255);
    ctx->beginPath();
    ctx->fillEllipse(ball_pos, 5.0);
    ctx->fill();

    ///auto fingers = camera->fingers;
    ///for (auto& finger : fingers)
    ///{
    ///    ctx->print() << finger.fingerId << ": (" << finger.x << ", " << finger.y << ")\n";
    ///}
    
    //ctx->print() << toString().c_str();
}

void Test1_Scene::onEvent(Event e)
{
    navigator.handleWorldNavigation(e, true);
}

//void Test_Scene::onPointerDown(PointerEvent e) {}
//void Test_Scene::onPointerUp(PointerEvent e) {}
//void Test_Scene::onPointerMove(PointerEvent e) {}
//void Test_Scene::onWheel(PointerEvent e) {}
//void Test_Scene::onKeyDown(KeyEvent e) {}
//void Test_Scene::onKeyUp(KeyEvent e) {}


SIM_END;

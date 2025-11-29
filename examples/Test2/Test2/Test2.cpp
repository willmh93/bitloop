#include "Test2.h"
///#include "CurvedSpace.h"
///#include "SpaceEngine.h"
///#include "EarthMoon.h"

SIM_DECLARE(Test2)


/// =========================
/// ======== Project ========
/// =========================

void Test2_Project_Vars::populateUI()
{
    ImGui::SliderInt("Viewport Count", &viewport_count, 1, 8);
}

void Test2_Project::projectPrepare(Layout& layout)
{
    /// Create multiple instance of a single Scene, mount to separate viewports
    layout << create<Test2_Scene>(1);
    //layout << create<Test::Test_Scene>(1);

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

void Test2_Scene_Attributes::populateUI()
{
    ImGui::Checkbox("Transform coordinates", &transform_coordinates); // updated in realtime
    ImGui::Checkbox("Scale Lines & Text", &scale_lines); // updated in realtime
    ImGui::Checkbox("Scale Sizes", &scale_sizes); // updated in realtime
    ImGui::Checkbox("Rotate Text", &rotate_text); // updated in realtime

    ImGui::SliderDouble("Camera Rotatation", &camera_rotation, 0.0, pi * 2.0); // updated in realtime
    ImGui::SliderDouble("Camera X",  &camera_x, -500.0, 500.0); // updated in realtime
    ImGui::SliderDouble("Camera Y",  &camera_y, -500.0, 500.0); // updated in realtime
    ImGui::SliderDouble("Zoom X",    &zoom_x, -5.0, 5.0); // updated in realtime
    ImGui::SliderDouble("Zoom Y",    &zoom_y, -5.0, 5.0); // updated in realtime
    ImGui::SliderDouble("Zoom Mult", &zoom_mult, 0.01, 100.0); // updated in realtime

    //static ImRect vr = { 0.0f, 0.8f, 0.8f, 0.0f };
    //ImSpline::SplineEditor("X/Y Spline", &sync(spline), &vr);
}

void Test2_Scene::sceneStart()
{
    //spline.create(100, {
    //    {0.0f, 0.0f}, {0.1f, 0.1f}, {0.2f, 0.2f},
    //    {0.3f, 0.3f}, {0.4f, 0.4f}, {0.5f, 0.5f},
    //    {0.6f, 0.6f}, {0.7f, 0.7f}, {0.8f, 0.8f}
    //});

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

void Test2_Scene::sceneMounted(Viewport*)
{
    // Initialize viewport
    //if (viewport->viewportIndex() == 0)
    //    //camera->setOriginViewportAnchor(Anchor::CENTER);
    //    camera->setOriginViewportAnchor(Anchor::TOP_LEFT);
    //else
    //camera->setOriginViewportAnchor(Anchor::TOP_LEFT);
    camera->setOriginViewportAnchor(Anchor::CENTER);

    //camera->focusWorldRect(0, 0, 300, 300);
}

void Test2_Scene::sceneDestroy()
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

// Find the nth prime number after a starting point
unsigned long long find_large_prime(unsigned long long start, int count) {
    unsigned long long candidate = start;
    int found = 0;
    int a = 5;
    while (found < count) {
        if (is_prime(candidate)) {
            ++found;
        }
        ++candidate;
        a += rand();
    }
    return candidate - 1;
}

void Test2_Scene::sceneProcess()
{
    // Process scene update
    ///for (int i = 0; i < 10; i++)
    ///{
    ///    unsigned long long prime = find_large_prime(2000000000000ul + rand()%100000ul, 1);
    ///}

    for (Particle& p : particles)
    {
        p.x += p.vx;
        p.y += p.vy;
        p.vx -= p.x * 0.0001;
        p.vy -= p.y * 0.0001;
    }

    //camera_rotation += 0.01;

    //static double a = 0.0;
    //spline[0].y = (float(sin(a) * 0.5 + 0.5));
    //a += 0.01;
}

void Test2_Scene::viewportProcess(Viewport*, double)
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
    {
        //camera->setPos(camera_x, camera_y);
        camera->setZoomX(zoom_x * zoom_mult);
        camera->setZoomY(zoom_y * zoom_mult);
        camera->setRotation(camera_rotation);
    }



    ///obj.align_x = 0.5;
    ///obj.align_y = 0.5;

    //obj.rotation += 0.01;

    //ball_pos.x = mouse->world_x;
    //ball_pos.y = mouse->world_y;
}



void Test2_Scene::viewportDraw(Viewport* ctx) const
{
    //camera->x += 1;
    //ctx->beginPath();
    //ctx->circle(0, 0, 5);
    //ctx->fill();

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


    camera->worldCoordinates(transform_coordinates);
    camera->scalingLines(scale_lines);
    camera->scalingSizes(scale_sizes);
    camera->rotatingText(rotate_text);

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

    

    /*ctx->setFillStyle(0, 255, 255);
    ctx->beginPath();
    ctx->circle(ball_pos, 10);
    ctx->fill();*/

    ///auto fingers = camera->fingers;
    ///for (auto& finger : fingers)
    ///{
    ///    ctx->print() << finger.fingerId << ": (" << finger.x << ", " << finger.y << ")\n";
    ///}
    
    //ctx->print() << toString().c_str();
}

void Test2_Scene::onEvent(Event e)
{
    if (this->ownsEvent(e))
        handleWorldNavigation(e, true);
}

//void Test_Scene::onPointerDown(PointerEvent e) {}
//void Test_Scene::onPointerUp(PointerEvent e) {}
//void Test_Scene::onPointerMove(PointerEvent e) {}
//void Test_Scene::onWheel(PointerEvent e) {}
//void Test_Scene::onKeyDown(KeyEvent e) {}
//void Test_Scene::onKeyUp(KeyEvent e) {}


SIM_END(Test2)

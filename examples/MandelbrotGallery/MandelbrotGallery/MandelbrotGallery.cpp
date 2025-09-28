#include "MandelbrotGallery.h"

SIM_BEG;

using namespace bl;

/// ─────────────────────── Project ───────────────────────

void MandelbrotGallery_Project::UI::sidebar()
{
    /// Remember: bl_pull / bl_push all used MandelbrotGallery_Project:: variables

    bl_pull(viewport_count); 
    ImGui::SliderInt("Viewport Count", &viewport_count, 1, 8);
    bl_push(viewport_count);
}

void MandelbrotGallery_Project::projectPrepare(Layout& layout)
{
    /// Create multiple instance of a single Scene, mount to separate viewports
    layout << create<Mandelbrot::Mandelbrot_Scene>({ "Heartbeat" });
    layout << create<Mandelbrot::Mandelbrot_Scene>({ "Clockwork" });
    layout << create<Mandelbrot::Mandelbrot_Scene>({ "Firework"  });
    layout << create<Mandelbrot::Mandelbrot_Scene>({ "Julia Island" });


    /// Or create a single Scene instance and view on multiple Viewports
    //auto* scene = create<MandelbrotGallery_Scene>();
    //for (int i = 0; i < viewport_count; ++i)
    //    layout << scene;
}

/// ─────────────────────── Scene ───────────────────────

void MandelbrotGallery_Scene::UI::sidebar()
{
    /// Remember: bl_pull / bl_push all used MandelbrotGallery_Scene:: variables

    if (ImGui::Section("View", true))
    {
        // Populate ImGui camera controls
        bl_pull(cam_view);
        cam_view.populateUI();
        bl_push(cam_view);
    }

    /// UI local options, no bl_pull / bl_push needed
    //ImGui::Checkbox("option", &test_popup_open);

    /// Scene variable
    //bl_pull(gravity);
    //ImGui::SliderDouble("gravity", &gravity, 0.0, 10.0);
    //bl_push(gravity);

    /// Queue MandelbrotGallery_Scene::foo() call
    //if (ImGui::Button("FOO"))
    //{
    //    bl_schedule([](MandelbrotGallery_Scene& scene) {
    //        scene.foo("BAR");
    //    });
    //}
}

void MandelbrotGallery_Scene::sceneStart()
{
    // Initialize scene
}

void MandelbrotGallery_Scene::sceneMounted(Viewport*)
{
    // Initialize viewport
    camera->setOriginViewportAnchor(Anchor::CENTER);
    //camera->focusWorldRect(0, 0, 100, 100);

    // Apply updates from imgui to camera view
    cam_view.read(camera);
}

void MandelbrotGallery_Scene::sceneDestroy()
{
    // Destroy scene (dynamically allocated resources, etc.)
}

void MandelbrotGallery_Scene::sceneProcess()
{
    // Apply updates from imgui to camera view
    cam_view.apply(camera);

    // Process scene update (once per scene)
}

void MandelbrotGallery_Scene::viewportProcess(
    [[maybe_unused]] Viewport* ctx,
    [[maybe_unused]] double dt)
{
    // Process scene update (per mounted viewport)
}

void MandelbrotGallery_Scene::viewportDraw(Viewport* ctx) const
{
    // Draw Scene on this viewport (only draw, don't modify sim states)
    ctx->drawWorldAxis();
}

void MandelbrotGallery_Scene::onEvent(Event e)
{
    if (handleWorldNavigation(e, true))
    {
        // Update camera view after handling navigation
        cam_view.read(e.ctx_owner()->camera);
    }
}

//void MandelbrotGallery_Scene::onPointerDown(PointerEvent e) {{}}
//void MandelbrotGallery_Scene::onPointerUp(PointerEvent e) {{}}
//void MandelbrotGallery_Scene::onPointerMove(PointerEvent e) {{}}
//void MandelbrotGallery_Scene::onWheel(PointerEvent e) {{}}
//void MandelbrotGallery_Scene::onKeyDown(KeyEvent e) {{}}
//void MandelbrotGallery_Scene::onKeyUp(KeyEvent e) {{}}

SIM_END;

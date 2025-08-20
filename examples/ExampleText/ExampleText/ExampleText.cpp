#include "ExampleText.h"

SIM_BEG(ExampleText)

using namespace BL;

/// ─────────────────────── Project ───────────────────────

void ExampleText_Project_Data::populateUI()
{
    ImGui::SliderInt("Viewport Count", &viewport_count, 1, 8);
}

void ExampleText_Project::projectPrepare(Layout& layout)
{
    /// Create multiple instance of a single Scene, mount to separate viewports
    layout << create<ExampleText_Scene>(viewport_count);

    /// Or create a single Scene instance and view on multiple Viewports
    //auto* scene = create<ExampleText_Scene>();
    //for (int i = 0; i < viewport_count; ++i)
    //    layout << scene;
}

/// ─────────────────────── Scene ───────────────────────

void ExampleText_Scene_Data::populateUI()
{
    if (ImGui::Section("View", true))
    {
        // imgui camera controls
        cam_view.populateUI();
    }

    //ImGui::Checkbox("option", &option);
    //ImGui::SliderDouble("value", &value, 0.0, 1.0);
}

void ExampleText_Scene::sceneStart()
{
    // Initialize scene
}

void ExampleText_Scene::sceneMounted(Viewport*)
{
    // Initialize viewport
    camera->setOriginViewportAnchor(Anchor::CENTER);
    //camera->focusWorldRect(0, 0, 100, 100);

    // Apply updates from imgui to camera view
    cam_view.read(camera);
}

void ExampleText_Scene::sceneDestroy()
{
    // Destroy scene (dynamically allocated resources, etc.)
}

void ExampleText_Scene::sceneProcess()
{
    // Apply updates from imgui to camera view
    cam_view.apply(camera); 

    // Process scene update
}

void ExampleText_Scene::viewportProcess(
    [[maybe_unused]] Viewport* ctx,
    [[maybe_unused]] double dt)
{
    // Process viewport update
}

void ExampleText_Scene::viewportDraw(Viewport* ctx) const
{
    // Draw Scene on this viewport (don't update simulation state here)
    ctx->drawWorldAxis();
}

void ExampleText_Scene::onEvent(Event e)
{
    if (e.ctx_owner())
    {
        // Handle camera navigation for viewport that captured the event
        e.ctx_owner()->camera.handleWorldNavigation(e, true);

        // Update camera view after handling navigation
        cam_view.read(e.ctx_owner()->camera);
    }
}

//void ExampleText_Scene::onPointerDown(PointerEvent e) {{}}
//void ExampleText_Scene::onPointerUp(PointerEvent e) {{}}
//void ExampleText_Scene::onPointerMove(PointerEvent e) {{}}
//void ExampleText_Scene::onWheel(PointerEvent e) {{}}
//void ExampleText_Scene::onKeyDown(KeyEvent e) {{}}
//void ExampleText_Scene::onKeyUp(KeyEvent e) {{}}

SIM_END(ExampleText)

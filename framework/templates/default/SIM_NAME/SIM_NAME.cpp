#include "{SIM_NAME}.h"

SIM_BEG;

using namespace bl;

/// ─────────────────────── Project ───────────────────────

void {SIM_NAME}_Project_Data::populateUI()
{
    ImGui::SliderInt("Viewport Count", &viewport_count, 1, 8);
}

void {SIM_NAME}_Project::projectPrepare(Layout& layout)
{
    /// Create multiple instance of a single Scene, mount to separate viewports
    layout << create<{SIM_NAME}_Scene>(viewport_count);

    /// Or create a single Scene instance and view on multiple Viewports
    //auto* scene = create<{SIM_NAME}_Scene>();
    //for (int i = 0; i < viewport_count; ++i)
    //    layout << scene;
}

/// ─────────────────────── Scene ───────────────────────

void {SIM_NAME}_Scene_Data::populateUI()
{
    if (ImGui::Section("View", true))
    {
        // imgui camera controls
        cam_view.populateUI();
    }

    //ImGui::Checkbox("option", &option);
    //ImGui::SliderDouble("value", &value, 0.0, 1.0);
}

void {SIM_NAME}_Scene::sceneStart()
{
    // Initialize scene
}

void {SIM_NAME}_Scene::sceneMounted(Viewport*)
{
    // Initialize viewport
    camera->setOriginViewportAnchor(Anchor::CENTER);
    //camera->focusWorldRect(0, 0, 100, 100);

    // Apply updates from imgui to camera view
    cam_view.read(camera);
}

void {SIM_NAME}_Scene::sceneDestroy()
{
    // Destroy scene (dynamically allocated resources, etc.)
}

void {SIM_NAME}_Scene::sceneProcess()
{
    // Apply updates from imgui to camera view
    cam_view.apply(camera);

    // Process scene update
}

void {SIM_NAME}_Scene::viewportProcess(
    [[maybe_unused]] Viewport* ctx,
    [[maybe_unused]] double dt)
{
    // Process viewport update
}

void {SIM_NAME}_Scene::viewportDraw(Viewport* ctx) const
{
    // Draw Scene on this viewport (don't update simulation state here)
    ctx->drawWorldAxis();
}

void {SIM_NAME}_Scene::onEvent(Event e)
{
    if (e.ctx_owner())
    {
        // Handle camera navigation for viewport that captured the event
        e.ctx_owner()->camera.handleWorldNavigation(e, true);

        // Update camera view after handling navigation
        cam_view.read(e.ctx_owner()->camera);
    }
}

//void {SIM_NAME}_Scene::onPointerDown(PointerEvent e) {{}}
//void {SIM_NAME}_Scene::onPointerUp(PointerEvent e) {{}}
//void {SIM_NAME}_Scene::onPointerMove(PointerEvent e) {{}}
//void {SIM_NAME}_Scene::onWheel(PointerEvent e) {{}}
//void {SIM_NAME}_Scene::onKeyDown(KeyEvent e) {{}}
//void {SIM_NAME}_Scene::onKeyUp(KeyEvent e) {{}}

SIM_END;

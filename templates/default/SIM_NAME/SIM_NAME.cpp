#include "{SIM_NAME}.h"

SIM_BEG;

using namespace bl;

/*  ─────────────────── Development tips ───────────────────

  - Any custom types synced with:  bl_pull(..)  bl_push(..)  bl_scoped(..)
    should have a cheap operator==(const T& rhs) to avoid unnecessary syncing.
    (Use hashing where possible(.)

*/


/// ─────────────────────── Project Setup ───────────────────────

void {SIM_NAME}_Project::UI::populate()
{
    /// Use bl_pull(..) / bl_push(..) on all used {SIM_NAME}_Project:: variables
    /// or use bl_scoped(..) to pull/push automatically in the current scope
    
    bl_scoped(viewport_count);
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

void {SIM_NAME}_Scene::UI::populate()
{
    /// Use bl_pull(..) / bl_push(..) on all used {SIM_NAME}_Scene:: variables
    /// or use bl_scoped(..) to pull/push automatically in the current scope.
    /// 
    /// Avoid static variables, they're shared across all scenes.

    bl_scoped(cam_view);

    if (ImGui::Section("View", true))
        cam_view.populateUI(); // Populate ImGui camera controls

    
    /// EXAMPLE: Scene variable input
    // bl_pull(gravity);
    // ImGui::SliderDouble("gravity", &gravity, 0.0, 10.0);
    // bl_push(gravity);

    /// EXAMPLE: Queue {SIM_NAME}_Scene::foo() call
    // if (ImGui::Button("FOO"))
    // {
    //     bl_schedule([]({SIM_NAME}_Scene& scene) {
    //         scene.foo("BAR");
    //     });
    // }

    /// EXAMPLE: UI-only options, no bl_pull / bl_push needed
    // ImGui::Checkbox("option", &test_popup_open);
}

void {SIM_NAME}_Scene::sceneStart()
{
    /// Initialize scene
}

void {SIM_NAME}_Scene::sceneMounted(Viewport*)
{
    /// Initialize viewport
    camera->setOriginViewportAnchor(Anchor::CENTER);
    //camera->focusWorldRect(0, 0, 100, 100);

    /// Apply updates from imgui to camera view
    cam_view.read(camera);
}

void {SIM_NAME}_Scene::sceneDestroy()
{
    /// Destroy scene (dynamically allocated resources, etc.)
}

void {SIM_NAME}_Scene::sceneProcess()
{
    /// Apply updates from imgui to camera view
    cam_view.apply(camera);

    /// Process entire scene once each frame (not per viewport)
}

void {SIM_NAME}_Scene::viewportProcess(
    [[maybe_unused]] Viewport* ctx,
    [[maybe_unused]] double dt)
{
    /// Process scene update (called once per mounted viewport each frame)
}

void {SIM_NAME}_Scene::viewportDraw(Viewport* ctx) const
{
    /// Draw Scene on this viewport (only draw, don't try to modify sim states)
    ctx->drawWorldAxis();
}

void {SIM_NAME}_Scene::onEvent(Event e)
{
    if (handleWorldNavigation(e, true))
    {
        /// Update camera view after handling navigation
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

#include "{SIM_NAME}.h"

SIM_BEG;

using namespace bl;

/// ─────────────────────── project setup ───────────────────────

void {SIM_NAME}_Project::UI::sidebar()
{
    /// use bl_pull(..) / bl_push(..) on all used {SIM_NAME}_Project:: variables
    /// or bl_scoped(..) to pull/push automatically in the current scope
    /// custom types should have:
    ///   - an operator==(const T&) overload to avoid unnecessary syncing (use hashing if possible)
    ///   - an assignment overload and copy constructor
    
    bl_scoped(viewport_count);
    ImGui::SliderInt("Viewport Count", &viewport_count, 1, 8);
}

void {SIM_NAME}_Project::projectPrepare(Layout& layout)
{
    /// create multiple instance of a single Scene, mount to separate viewports
    for (int i = 0; i < viewport_count; i++)
        layout << create<{SIM_NAME}_Scene>();

    /// or create a single Scene instance and view on multiple Viewports
    //auto* scene = create<{SIM_NAME}_Scene>();
    //for (int i = 0; i < viewport_count; ++i)
    //    layout << scene;
}

/// ─────────────────────── scene ───────────────────────

void {SIM_NAME}_Scene::UI::sidebar()
{
    /// use bl_pull(..) / bl_push(..) on all used {SIM_NAME}_Project:: variables
    /// or bl_scoped(..) to pull/push automatically in the current scope
    /// custom types should have:
    ///   - an operator==(const T&) overload to avoid unnecessary syncing (use hashing if possible)
    ///   - an assignment overload and copy constructor

    if (ImGui::CollapsingHeaderBox("View", true))
    {
        // imgui camera controls
        bl_scoped(camera);
        camera.populateUI();
        ImGui::EndCollapsingHeaderBox();
    }

    /// example: Scene variable input
    // bl_pull(gravity);
    // ImGui::SliderDouble("gravity", &gravity, 0.0, 10.0);
    // bl_push(gravity);

    /// example: Queue {SIM_NAME}_Scene::foo() call
    // if (ImGui::Button("FOO"))
    // {
    //     bl_schedule([]({SIM_NAME}_Scene& scene) {
    //         scene.foo("BAR");
    //     });
    // }

    /// example: UI-only, no sync needed
    // ImGui::Checkbox("option", &test_popup_open);
}

void {SIM_NAME}_Scene::sceneStart()
{
    /// initialize scene
}

void {SIM_NAME}_Scene::sceneMounted(Viewport* ctx)
{
    // initialize viewport
    camera.setSurface(ctx);
    camera.setOriginViewportAnchor(Anchor::CENTER);
    //camera.focusWorldRect(-100, -100, 100, 100);
    //camera.uiSetCurrentAsDefault();

    navigator.setTarget(camera);
    navigator.setDirectCameraPanning(true);
}

void {SIM_NAME}_Scene::sceneDestroy()
{
    /// destroy scene (dynamically allocated resources, etc.)
}

void {SIM_NAME}_Scene::sceneProcess()
{
    /// process scene once each frame (not per viewport)
}

void {SIM_NAME}_Scene::viewportProcess(
    [[maybe_unused]] Viewport* ctx,
    [[maybe_unused]] double dt)
{
    /// process scene update (called once per mounted viewport each frame)
}

void {SIM_NAME}_Scene::viewportDraw(Viewport* ctx) const
{
    /// draw scene on this viewport (no modifying sim state)
    ctx->transform(camera.getTransform());
    ctx->drawWorldAxis();
}

void {SIM_NAME}_Scene::onEvent(Event e)
{
    navigator.handleWorldNavigation(e, true);
}

//void {SIM_NAME}_Scene::onPointerDown(PointerEvent e) {{}}
//void {SIM_NAME}_Scene::onPointerUp(PointerEvent e) {{}}
//void {SIM_NAME}_Scene::onPointerMove(PointerEvent e) {{}}
//void {SIM_NAME}_Scene::onWheel(PointerEvent e) {{}}
//void {SIM_NAME}_Scene::onKeyDown(KeyEvent e) {{}}
//void {SIM_NAME}_Scene::onKeyUp(KeyEvent e) {{}}

SIM_END;

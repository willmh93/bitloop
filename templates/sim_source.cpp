#include "{CLASS_NAME}.h"
SIM_DECLARE({CLASS_NAME}, "New Projects", "{SIM_NAME}")

/// =========================
/// ======== Project ========
/// =========================

void {CLASS_NAME}_Project::projectAttributes()
{{
    ImGui::SliderInt("Panel Count", Initial(&panel_count), 1, 8);
}}

void {CLASS_NAME}_Project::projectPrepare(Layout& layout)
{{
    /// Create multiple instance of a single Scene, mount to separate viewports
    layout << create<{CLASS_NAME}_Scene>(viewport_count);

    /// Or create a single Scene instance and view on multiple Viewports
    //auto* scene = create<{CLASS_NAME}_Scene>();
    //for (int i = 0; i < viewport_count; ++i)
    //    layout << scene;
}}

/// =========================
/// ========= Scene =========
/// =========================

void {CLASS_NAME}_Scene::sceneAttributes()
{{
    //--- Updating values just prior to sceneStart with: Initial(&var) ---//

    //ImGui::Checkbox("Starting Flag", Initial(&var1));                
    //ImGui::SliderDouble("Starting Double", Initial(&var3), 0.0, 1.0);

    //--- Updated in realtime ---//

    //ImGui::SliderDouble("Realtime Double", &var2, 0.0, 1.0); 
    
}}

void {CLASS_NAME}_Scene::sceneStart()
{{
    /// Initialize Scene
}}

void {CLASS_NAME}_Scene::sceneDestroy()
{{
    /// Destroy Scene
}}

void {CLASS_NAME}_Scene::sceneMounted(Viewport* viewport)
{{
    /// Initialize viewport (after sceneStart)
    camera->setOriginViewportAnchor(Anchor::CENTER);

    // Focus on {0,0,300,300}
    camera->focusWorldRect(0, 0, 300, 300);

    // Clamp Viewport Size between {150,150} and {600,600}
    camera->setRelativeZoomRange(0.5, 2); 
}}

void {CLASS_NAME}_Scene::sceneProcess()
{{
    /// Process Scene update
}}

void {CLASS_NAME}_Scene::viewportProcess(Viewport* ctx)
{{
    /// Process Viewports running this Scene
}}

void {CLASS_NAME}_Scene::viewportDraw(Viewport* ctx)
{{
    /// Draw Scene to Viewport
    ctx->drawWorldAxis();
}}

/// User Interaction

void Test_Scene::onEvent(Event e)
{
    handleWorldNavigation(e, true);
}

void {CLASS_NAME}_Scene::onPointerDown(PointerEvent e) {{}}
void {CLASS_NAME}_Scene::onPointerUp(PointerEvent e) {{}}
void {CLASS_NAME}_Scene::onPointerMove(PointerEvent e) {{}}
void {CLASS_NAME}_Scene::onWheel(PointerEvent e) {{}}
void {CLASS_NAME}_Scene::onKeyDown(KeyEvent e) {{}}
void {CLASS_NAME}_Scene::onKeyUp(KeyEvent e) {{}}

SIM_END({CLASS_NAME})
#include "ExampleText.h"

SIM_BEG;
using namespace bl;

/// ─────────────────────── Project ───────────────────────

void ExampleText_Project::UI::sidebar()
{
    bl_scoped(viewport_count);
    ImGui::SliderInt("Viewport Count", &viewport_count, 1, 8);
}

void ExampleText_Project::projectPrepare(Layout& layout)
{
    /// Create multiple instance of a single Scene, mount to separate viewports
    for (int i = 0; i < viewport_count; i++)
        layout << create<ExampleText_Scene>();

    /// Or create a single Scene instance and view on multiple Viewports
    //auto* scene = create<ExampleText_Scene>();
    //for (int i = 0; i < viewport_count; ++i)
    //    layout << scene;
}

/// ─────────────────────── Scene ───────────────────────

void ExampleText_Scene::UI::sidebar()
{
    if (ImGui::Section("View", true))
    {
        bl_scoped(camera);
        camera.populateUI(); // imgui camera controls
    }
    
    if (ImGui::Section("Options", true))
    {
        bl_scoped(transform_coordinates);
        bl_scoped(scale_lines);
        bl_scoped(scale_sizes);
        bl_scoped(scale_text);
        bl_scoped(rotate_text);

        ImGui::Checkbox("Transform coordinates", &transform_coordinates);
        ImGui::Checkbox("Scale Lines", &scale_lines);
        ImGui::Checkbox("Scale Sizes", &scale_sizes);
        ImGui::Checkbox("Scale Text",  &scale_text);
        ImGui::Checkbox("Rotate Text", &rotate_text);
    }
}

void ExampleText_Scene::sceneStart()
{
    // Initialize scene
    fonts.push_back(NanoFont::create("/data/fonts/NK57 Monospace Sc Bk.otf"));
    fonts.push_back(NanoFont::create("/data/fonts/Baileys Car.otf"));
    fonts.push_back(NanoFont::create("/data/fonts/Barbatrick.otf"));
    fonts.push_back(NanoFont::create("/data/fonts/Baveuse 3d.otf"));
    fonts.push_back(NanoFont::create("/data/fonts/Dacquoise.otf"));
    fonts.push_back(NanoFont::create("/data/fonts/Deftone Stylus.otf"));
    fonts.push_back(NanoFont::create("/data/fonts/Degrassi.otf"));
    fonts.push_back(NanoFont::create("/data/fonts/Deluxe Ducks.otf"));
    fonts.push_back(NanoFont::create("/data/fonts/Die Nasty.otf"));
    fonts.push_back(NanoFont::create("/data/fonts/Iomanoid.otf"));
    fonts.push_back(NanoFont::create("/data/fonts/Jandles.otf"));
    fonts.push_back(NanoFont::create("/data/fonts/Metal Lord.otf"));
    fonts.push_back(NanoFont::create("/data/fonts/Paint Boy.otf"));
    fonts.push_back(NanoFont::create("/data/fonts/Waker.otf"));
    fonts.push_back(NanoFont::create("/data/fonts/Robokoz.otf"));
    fonts.push_back(NanoFont::create("/data/fonts/Zeroes One.otf"));
    fonts.push_back(NanoFont::create("/data/fonts/Lunasol Aurora.otf"));
    fonts.push_back(NanoFont::create("/data/fonts/Street Cred.otf"));

}

void ExampleText_Scene::sceneMounted(Viewport* ctx)
{
    // Initialize viewport
    camera.setSurface(ctx);
    camera.setOriginViewportAnchor(Anchor::CENTER);
    camera.focusWorldRect(-100, -100, 1000, 1000);

    navigator.setTarget(camera);
    navigator.setDirectCameraPanning(true);

}

void ExampleText_Scene::sceneDestroy()
{
    // Destroy scene (dynamically allocated resources, etc.)
}

void ExampleText_Scene::sceneProcess()
{
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
    ctx->setTransform(camera.getTransform());
    ctx->drawWorldAxis();

    ctx->scale(10);

    ctx->worldCoordinates(transform_coordinates);
    ctx->scalingLines(scale_lines);
    ctx->scalingSizes(scale_sizes);
    ctx->scalingText(scale_text);
    ctx->rotatingText(rotate_text);

    double font_size = 32;

    ctx->setFontSize(font_size);
    ctx->setFillStyle(255, 255, 255);
    ctx->setTextAlign(TextAlign::ALIGN_LEFT);
    for (size_t i = 0; i < fonts.size(); i++)
    {
        double y = (double)i * font_size + 50.0;
        ctx->setFont(fonts[i]);
        //ctx->fillText("The quick brown fox jumps over the lazy dog", 20, y);
        ctx->fillText("The quick brown fox jumps over the lazy dog", 50.0, y);

    }
}

void ExampleText_Scene::onEvent(Event e)
{
    navigator.handleWorldNavigation(e, true);
}

//void ExampleText_Scene::onPointerDown(PointerEvent e) {{}}
//void ExampleText_Scene::onPointerUp(PointerEvent e) {{}}
//void ExampleText_Scene::onPointerMove(PointerEvent e) {{}}
//void ExampleText_Scene::onWheel(PointerEvent e) {{}}
//void ExampleText_Scene::onKeyDown(KeyEvent e) {{}}
//void ExampleText_Scene::onKeyUp(KeyEvent e) {{}}

SIM_END;

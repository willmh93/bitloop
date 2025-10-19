#include "Tiger.h"
#include "draw_tiger.h"

SIM_BEG;

using namespace bl;

/// ─────────────────────── Project ───────────────────────

void Tiger_Project::UI::sidebar()
{
    bl_scoped(viewport_count);

    ImGui::SliderInt("Viewport Count", &viewport_count, 1, 8);
}

void Tiger_Project::projectPrepare(Layout& layout)
{
    layout << create<Tiger_Scene>(viewport_count);
}

/// ─────────────────────── Scene ───────────────────────

void Tiger_Scene::UI::sidebar()
{
    bl_scoped(transform_coordinates);
    bl_scoped(scale_lines);
    bl_scoped(scale_sizes);
    bl_scoped(rotate_text);

    ImGui::Checkbox("Transform coordinates", &transform_coordinates);
    ImGui::Checkbox("Scale Lines", &scale_lines);
    ImGui::Checkbox("Scale Sizes", &scale_sizes);
    ImGui::Checkbox("Rotate Text", &rotate_text);

    if (ImGui::Button("Roar"))
    {
        bl_schedule([](Tiger_Scene& scene) {
            scene.woof("ROAR!");
        });
    }

    if (ImGui::Section("View", true)) 
    {
        // imgui camera controls
        bl_scoped(camera);
        camera.populateUI();
    }
}

void Tiger_Scene::sceneStart()
{}

void Tiger_Scene::sceneMounted(Viewport* ctx)
{
    // Initialize viewport
    camera.setSurface(ctx);
    camera.setOriginViewportAnchor(Anchor::CENTER);
    camera.focusWorldRect(-100, -100, 1000, 1000);

    navigator.setTarget(camera);
    navigator.setDirectCameraPanning(true);
}

void Tiger_Scene::sceneDestroy()
{}

void Tiger_Scene::sceneProcess()
{
}

void Tiger_Scene::viewportProcess(Viewport*, double)
{
    // Process your scene...
    ///camera.setRotation(camera.rotation() + 0.01);
}

void Tiger_Scene::viewportDraw(Viewport* ctx) const
{
    ctx->transform(camera.getTransform());
    ctx->drawWorldAxis();

    //ctx->scale(2);

    ctx->worldCoordinates(transform_coordinates);
    ctx->scalingLines(scale_lines);
    ctx->scalingSizes(scale_sizes);

    draw_tiger(ctx);

    ctx->print() << "Bark: " << woof_noise;
}

void Tiger_Scene::onEvent(Event e)
{
    navigator.handleWorldNavigation(e, true);
}

SIM_END;

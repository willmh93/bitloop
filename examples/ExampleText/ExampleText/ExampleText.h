#pragma once
#include <project.h>

SIM_BEG(ExampleText)

using namespace BL;

struct ExampleText_Scene_Data : VarBuffer
{
    CameraViewController cam_view;
    
    bool transform_coordinates = true;
    bool scale_lines = true;
    bool scale_sizes = true;
    bool scale_text  = true;
    bool rotate_text = true;

    void registerSynced() override
    {
        // auto-synced between worker/GUI thread
        sync(cam_view);

        sync(transform_coordinates);
        sync(scale_lines);
        sync(scale_sizes);
        sync(scale_text);
        sync(rotate_text);
    }
    void populateUI() override;
};

struct ExampleText_Scene : public Scene<ExampleText_Scene_Data >
{
    /// ─────── Provide default Scene launch config ─────── 
    struct Config {};
    ExampleText_Scene(Config& info [[maybe_unused]]) {}

    /// ─────── Scene variables ───────
    std::vector<std::shared_ptr<NanoFont>> fonts;

    /// ─────── Scene methods ───────
    void sceneStart() override;
    void sceneMounted(Viewport* viewport) override;
    void sceneDestroy() override;
    void sceneProcess() override;

    /// ─────── Viewport methods ─────── 
    void viewportProcess(Viewport* ctx, double dt) override;
    void viewportDraw(Viewport* ctx) const override;

    /// ─────── Input handling ─────── 
    void onEvent(Event e) override; // All event types

    /// Filters for specific input events
    //void onPointerDown(PointerEvent e) override;
    //void onPointerUp(PointerEvent e) override;
    //void onPointerMove(PointerEvent e) override;
    //void onWheel(PointerEvent e) override;
    //void onKeyDown(KeyEvent e) override;
    //void onKeyUp(KeyEvent e) override;

    /// ─────── Your custom methods ───────
    // void customMethod();
};

struct ExampleText_Project_Data : public VarBuffer
{
    int viewport_count = 1;

    void populateUI();
    void registerSynced()
    {
        sync(viewport_count);
    }
};

struct ExampleText_Project : public Project<ExampleText_Project_Data>
{
    static ProjectInfo info()
    {
        // Categorize your project
        return ProjectInfo({ "Tests", "Draw Text" });
    }

    void projectPrepare(Layout& layout) override;
};

SIM_END(ExampleText)

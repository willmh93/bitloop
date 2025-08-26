#pragma once
#include <bitloop.h>

SIM_BEG;

using namespace BL;

struct Tiger_Scene_Data : VarBuffer
{
    bool transform_coordinates = true;
    bool scale_lines = true;
    bool scale_sizes = true;
    bool rotate_text = true;

    CameraViewController cam_view;
    
    void registerSynced() override
    {
        sync(transform_coordinates);
        sync(scale_lines);
        sync(scale_sizes);
        sync(rotate_text);

        sync(cam_view);
    }
    void populateUI() override;
};

struct Tiger_Scene : public Scene<Tiger_Scene_Data>
{
    /// ─────── Provide default Scene launch config ─────── 
    struct Config { 
        // double speed = 10;
    };
    Tiger_Scene(Config& info [[maybe_unused]])
        // : speed(info.speed)  /// Initialize variables below from config
    {}

    /// ─────── Scene variables ─────── 
    // double speed;

    /// ─────── Scene methods ─────── 
    void sceneStart() override;
    void sceneMounted(Viewport* viewport) override;
    void sceneDestroy() override;
    void sceneProcess() override;

    /// ─────── Viewport methods ─────── 
    void viewportProcess(Viewport* ctx, double dt) override;
    void viewportDraw(Viewport* ctx) const override;

    /// ─────── Input handling ─────── 
    void onEvent(Event e) override;

    /// ─────── Your custom methods ─────── 


};

struct Tiger_Project_Data : public VarBuffer
{
    int viewport_count = 1;

    void populateUI();
    void registerSynced()
    {
        sync(viewport_count);
    }
};

struct Tiger_Project : public Project<Tiger_Project_Data>
{
    static ProjectInfo info()
    {
        return ProjectInfo({ "Tests", "Draw Tiger (vector graphics)" });
    }

    void projectPrepare(Layout& layout) override;
};

SIM_END;

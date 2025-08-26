#pragma once
#include <bitloop.h>

SIM_BEG;

using namespace BL;

struct {SIM_NAME}_Scene_Data : VarBuffer
{
    CameraViewController cam_view;
    //double speed = 0.0;

    void registerSynced() override
    {
        /// synced between worker/GUI thread
        sync(cam_view);
        //sync(speed);
    }
    void populateUI() override;
};

struct {SIM_NAME}_Scene : public Scene<{SIM_NAME}_Scene_Data >
{
    /// ─────── Provide default launch config ───────
    struct Config {
        // double gravity = 9.8;
    };

    {SIM_NAME}_Scene(Config& info [[maybe_unused]])
        // : gravity(info.gravity)
    {}

    /// ─────── Scene variables ───────
    // double gravity;

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

    // void onPointerDown(PointerEvent e) override;
    // void onPointerUp(PointerEvent e) override;
    // void onPointerMove(PointerEvent e) override;
    // void onWheel(PointerEvent e) override;
    // void onKeyDown(KeyEvent e) override;
    // void onKeyUp(KeyEvent e) override;

    /// ─────── Your custom methods ───────
    // void customMethod();
};

struct {SIM_NAME}_Project_Data : public VarBuffer
{
    int viewport_count = 1;

    void populateUI();
    void registerSynced()
    {
        sync(viewport_count);
    }
};

struct {SIM_NAME}_Project : public Project<{SIM_NAME}_Project_Data>
{
    static ProjectInfo info()
    {
        // Categorize your project
        return ProjectInfo({ "New Projects", "{SIM_NAME}" });
    }

    void projectPrepare(Layout& layout) override;
};

SIM_END;

#pragma once
#include <bitloop.h>

SIM_BEG;

using namespace bl;

struct {SIM_NAME}_Scene : public Scene<{SIM_NAME}_Scene>
{
    /// ─────── Default launch config (overridable by Project) ───────
    struct Config { /* double gravity = 9.8; */ };

    {SIM_NAME}_Scene(Config& info [[maybe_unused]])
        // : gravity(info.gravity)
    {}

    /// ─────── Thread-safe UI for editing Scene inputs with ImGui ───────
    struct UI : Interface
    {
        using Interface::Interface;
        void populate();

        /// ─────── Your UI-only variables ───────
        // bool test_popup_open = false;
    };

    /// ─────── Your variables ───────
    CameraViewController cam_view;
    // double gravity;

    /// ─────── Your methods ───────
    // void customMethod();

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
};

struct {SIM_NAME}_Project : public Project<{SIM_NAME}_Project>
{
    static ProjectInfo info() {
        // Categorize your project
        return ProjectInfo({ "New Projects", "{SIM_NAME}" });
    }

    struct UI : Interface {
        using Interface::Interface;
        void populate();
    };

    int viewport_count = 1;

    void projectPrepare(Layout& layout) override;
};

SIM_END;

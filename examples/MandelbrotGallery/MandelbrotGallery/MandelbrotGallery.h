#pragma once
#include <bitloop.h>

#include "Mandelbrot/Mandelbrot.h"

SIM_BEG;

using namespace bl;

struct MandelbrotGallery_Scene : public Scene<MandelbrotGallery_Scene>
{
    /// ─────── Default launch config (overridable by Project) ───────
    struct Config { /* double gravity = 9.8; */ };

    MandelbrotGallery_Scene(Config& info [[maybe_unused]])
        // : gravity(info.gravity)
    {}

    /// ─────── Thread-safe UI for editing Scene inputs with ImGui ───────
    struct UI : Interface
    {
        using Interface::Interface;
        void sidebar();

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

struct MandelbrotGallery_Project : public Project<MandelbrotGallery_Project>
{
    static ProjectInfo info()
    {
        // Categorize your project
        return ProjectInfo({ "Fractal", "Mandelbrot", "Gallery Composite" });
    }

    struct UI : Interface
    {
        using Interface::Interface;
        void sidebar();
    };

    int viewport_count = 1;

    void projectPrepare(Layout& layout) override;
};

SIM_END;

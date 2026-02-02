#pragma once

#include <bitloop.h>
SIM_BEG;

using namespace bl;

struct Particle : public DVec2
{
    double fx = 0, fy = 0;
    double vx, vy;
    Particle(double x, double y, double vx, double vy)
        : Vec2(x, y), vx(vx), vy(vy)
    {}
};

struct Test1_Scene : public Scene<Test1_Scene>
{
    // --- Custom Launch Config Example ---
    struct Config
    {
        //double speed = 10.0;
    };

    Test1_Scene(Config&) //:
        //speed(info.speed)
    {}

    struct UI : DoubleBufferedAccessor
    {
        using DoubleBufferedAccessor::DoubleBufferedAccessor;
        void sidebar();
    };

    DVec2 ball_pos = { 0, 0 };
    std::vector<Particle> particles;

    bool transform_coordinates = true;
    bool scale_lines = true;
    bool scale_sizes = true;
    bool rotate_text = true;

    CameraInfo camera;
    CameraNavigator navigator;

    // Scene management
    //void _sceneAttributes() override;
    void sceneStart() override;
    void sceneMounted(Viewport* viewport) override;
    void sceneDestroy() override;

    // --- Simulation processing ---
    void sceneProcess() override;

    // Viewport handling
    void viewportProcess(Viewport* ctx, double dt) override;
    void viewportDraw(Viewport* ctx) const override;

    // Input
    void onEvent(Event e) override;
    //void onPointerDown(PointerEvent e) override;
    //void onPointerUp(PointerEvent e) override;
    //void onPointerMove(PointerEvent e) override;
    //void onWheel(PointerEvent e) override;
    //void onKeyDown(KeyEvent e) override;
    //void onKeyUp(KeyEvent e) override;
};

struct Test1_Project : public Project<Test1_Project>
{
    static ProjectInfo info() {
        return ProjectInfo({ "Framework Tests", "Test A" });
    }

    struct UI : DoubleBufferedAccessor
    {
        using DoubleBufferedAccessor::DoubleBufferedAccessor;
        void sidebar();
    };

    int viewport_count = 1;

    void projectPrepare(Layout& layout) override;
};

SIM_END;

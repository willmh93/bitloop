#pragma once

#include <bitloop.h>
SIM_BEG(Test1)

using namespace BL;

struct Particle : public DVec2
{
    double fx = 0, fy = 0;
    double vx, vy;
    Particle(double x, double y, double vx, double vy)
        : Vec2(x, y), vx(vx), vy(vy)
    {}
};

/*struct Test_Scene_Attributes : public VarBuffer
{
    sync_struct
    {
        bool transform_coordinates = true;
        bool scale_lines = true;
        bool scale_sizes = true;
        bool rotate_text = true;

        double camera_x = 0;
        double camera_y = 0;
        double camera_rotation = 0;
        double zoom_x = 1;
        double zoom_y = 1;
    }
    sync_end;

    void populate();
    void copyFrom(const Test_Scene_Attributes& rhs)
    {
        transform_coordinates = rhs.transform_coordinates;
        scale_lines = rhs.scale_lines;
        scale_sizes = rhs.scale_sizes;
        rotate_text = rhs.rotate_text;
        camera_x = rhs.camera_x;
        camera_y = rhs.camera_y;
        camera_rotation = rhs.camera_rotation;
        zoom_x = rhs.zoom_x;
        zoom_y = rhs.zoom_y;
    }
};*/

struct Test1_Scene_Attributes : VarBuffer
{
    bool transform_coordinates = true;
    bool scale_lines = true;
    bool scale_sizes = true;
    bool rotate_text = true;

    double camera_x = 0;
    double camera_y = 0;
    double camera_rotation = 0;
    double zoom_x = 1;
    double zoom_y = 1;
    double zoom_mult = 1;

    void registerSynced() override
    {
        sync(transform_coordinates);
        sync(scale_lines);
        sync(scale_sizes);
        sync(rotate_text);
        sync(camera_x);
        sync(camera_y);
        sync(camera_rotation);
        sync(zoom_x);
        sync(zoom_y);
        sync(zoom_mult);
    }
    void populateUI() override;
};

struct Test1_Scene : public Scene<Test1_Scene_Attributes>
{
    DVec2 ball_pos = { 0, 0 };
    std::vector<Particle> particles;

    // --- Custom Launch Config Example ---
    struct Config
    {
        //double speed = 10.0;
    };

    Test1_Scene(Config&) //:
        //speed(info.speed)
    {}

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

struct Test1_Project_Vars : public VarBuffer
{
    int viewport_count = 1;

    void populateUI();
    void registerSynced()
    {
        sync(viewport_count);
    }
};

struct Test1_Project : public Project<Test1_Project_Vars>
{
    //static std::vector<std::string> categorize() {
    //    return { "Framework Tests", "Test A" };
    //}

    static ProjectInfo info()
    {
        return ProjectInfo({ "Framework Tests", "Test A" });
    }

    void projectPrepare(Layout& layout) override;
};

SIM_END(Test1)

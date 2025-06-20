#ifndef TEST_H
#define TEST_H
#include "project.h"
SIM_BEG(Test)

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
        bool scale_lines_text = true;
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
        scale_lines_text = rhs.scale_lines_text;
        scale_sizes = rhs.scale_sizes;
        rotate_text = rhs.rotate_text;
        camera_x = rhs.camera_x;
        camera_y = rhs.camera_y;
        camera_rotation = rhs.camera_rotation;
        zoom_x = rhs.zoom_x;
        zoom_y = rhs.zoom_y;
    }
};*/

struct Test_Scene_Attributes : VarBuffer
{
    double speed;

    DVec2 ball_pos = { 0, 0 };

    std::vector<Particle> particles;

    bool transform_coordinates = true;
    bool scale_lines_text = true;
    bool scale_sizes = true;
    bool rotate_text = true;

    double camera_x = 0;
    double camera_y = 0;
    double camera_rotation = 0;
    double zoom_x = 1;
    double zoom_y = 1;

    void populate() override;
};

struct Test_Scene : public Scene<Test_Scene_Attributes>
{
    // --- Custom Launch Config Example ---
    struct Config
    {
        //double speed = 10.0;
    };

    Test_Scene(Config&) //:
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

struct Test_Project_Vars : public VarBuffer
{
    int viewport_count = 1;

    void populate();
    void registerSynced()
    {
        sync(viewport_count);
    }
};

struct Test_Project : public Project<Test_Project_Vars>
{
    void projectPrepare(Layout& layout) override;
};

SIM_END(Test)
#endif

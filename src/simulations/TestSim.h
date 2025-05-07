#ifndef TEST_H
#define TEST_H
#include "project.h"
SIM_BEG(Test)

struct Particle : public Vec2
{
    double fx = 0, fy = 0;
    double vx, vy;
    Particle(double x, double y, double vx, double vy)
        : Vec2(x, y), vx(vx), vy(vy)
    {}
};


struct Test_Scene_Attributes : public VarBuffer<Test_Scene_Attributes>
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

    void populate(Test_Scene_Attributes &dst) override;
    void copyFrom(const Test_Scene_Attributes& rhs) override
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
};

struct Test_Scene : public Scene<Test_Scene_Attributes>
{
    // --- Custom Launch Config Example ---
    struct Config
    {
        double speed = 10.0;
    };

    Test_Scene(Config& info) :
        speed(info.speed)
    {}

    double speed;

    Vec2 ball_pos = { 0, 0 };

    std::vector<Particle> particles;

    //

    // Scene management
    //void populate() override;
    void sceneStart() override;
    void sceneMounted(Viewport* viewport) override;
    void sceneDestroy() override;

    // --- Simulation processing ---
    void sceneProcess() override;

    // Viewport handling
    void viewportProcess(Viewport* ctx) override;
    void viewportDraw(Viewport* ctx) override;

    // Input
    void onEvent(Event& e) override;
};

struct Test_Project_Vars : public VarBuffer<Test_Project_Vars>
{
    int viewport_count = 1;
    
    void populate(Test_Project_Vars &dst) override;
};

struct Test_Project : public ProjectBase
{
    void projectPrepare() override;
};

SIM_END(Test)
#endif

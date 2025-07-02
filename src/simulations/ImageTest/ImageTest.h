#ifndef TEST_H
#define TEST_H
#include "project.h"

//extern "C" void ImageTest_ForceLink();

SIM_BEG(ImageTest)


struct ImageTest_Scene_Attributes : VarBuffer
{
    void populate() override;
};

struct ImageTest_Scene : public Scene<ImageTest_Scene_Attributes>
{
    // --- Custom Launch Config Example ---
    struct Config
    {
        //double speed = 10.0;
    };

    ImageTest_Scene(Config&) //:
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

struct ImageTest_Project_Vars : public VarBuffer
{
    int viewport_count = 1;

    void populate();
    void registerSynced()
    {
        sync(viewport_count);
    }
};

struct ImageTest_Project : public Project<ImageTest_Project_Vars>
{
    void projectPrepare(Layout& layout) override;
};

SIM_END(ImageTest)
#endif

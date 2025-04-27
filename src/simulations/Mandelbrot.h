#ifndef MANDELBROT_H
#define MANDELBROT_H
#include "project.h"
SIM_BEG(Mandelbrot)


struct Mandelbrot_Scene : public Scene
{
    // --- Custom Launch Config ---
    struct Config
    {
        //double speed = 10.0;
    };

    Mandelbrot_Scene(Config& info) //:
        //speed(info.speed)
    {}

    // Scene management
    void sceneAttributes() override;
    void sceneStart() override;
    void sceneMounted(Viewport* viewport) override;
    void sceneDestroy() override;

    // --- Simulation processing ---
    void sceneProcess() override;

    // Viewport handling
    void viewportProcess(Viewport* ctx) override;
    void viewportDraw(Viewport* ctx) override;

    // Input
    ///void mouseDown() override;
    ///void mouseUp() override;
    ///void mouseMove() override;
    ///void mouseWheel() override;
};

struct Mandelbrot_Project : public Project
{
    int viewport_count = 1;

    void projectAttributes() override;
    void projectPrepare() override;
};

SIM_END(Mandelbrot)
#endif

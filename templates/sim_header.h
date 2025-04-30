#pragma once
#include "Project.h"

SIM_BEG({CLASS_NAME})

struct {CLASS_NAME}_Scene : public Scene
{{
/*  // --- Custom Launch Config Example ---
       
    struct Config
    {{
        double speed = 10.0;
    }};

    {CLASS_NAME}_Scene(Config& info) :
        speed(info.speed)
    {{}}

    double speed;
*/

    // --- Variables ---
    

    // --- Scene management ---
    void sceneAttributes() override;
    void sceneStart() override;
    //void sceneStop() override;
    void sceneDestroy() override;
    void sceneMounted(Viewport* viewport) override;

    // --- Update methods ---
    
    void sceneProcess() override;

    // --- Shaders ---
    //void loadShaders() override;

    // --- Viewport ---
    void viewportProcess(Viewport* ctx) override;
    void viewportDraw(Viewport* ctx) override;

    // --- Input ---
    void mouseDown() override;
    void mouseUp() override;
    void mouseMove() override;
    void mouseWheel() override;
}};

struct {CLASS_NAME}_Project : public Project
{{
    int panel_count = 1;

    void projectAttributes() override;
    void projectPrepare() override;
    //void projectStart() override;
    //void projectStop() override;
    //void projectDestroy() override;

}};

SIM_END({CLASS_NAME})
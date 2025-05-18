#pragma once
#include "Project.h"

SIM_BEG({CLASS_NAME})

struct {CLASS_NAME}_Scene_Vars : public VarBuffer<{CLASS_NAME}_Scene_Primitives>
{{
    void populate() override;
    void copyFrom(const {CLASS_NAME}_Scene_Vars& rhs) override
    {{
        /// ===== Control how data is copied between buffers =====
        ///
        ///    NOTE: It is *UNSAFE* to store raw pointers to any data contained inside this buffer
        ///          as operator=() will likely invalidate any dangling pointers. Take care when
        ///          using third party pointers
        ///
        *this = rhs;
    }}
}};

struct {CLASS_NAME}_Scene : public Scene<{CLASS_NAME}_Scene_Vars>
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

struct {CLASS_NAME}_Project_Vars : public VarBuffer<_Project_

struct {CLASS_NAME}_Project : public Project<{CLASS_NAME}_Project_Vars>
{{
    int panel_count = 1;

    void projectAttributes() override;
    void projectPrepare() override;
    //void projectStart() override;
    //void projectStop() override;
    //void projectDestroy() override;

}};

SIM_END({CLASS_NAME})
#pragma once

#include <imgui.h>
#include <SDL2/SDL.h>
#include "platform.h"

void dpiDebugInfo()
{
    ImGuiIO& io = ImGui::GetIO();

    //ImVec2 initSize(io.DisplaySize.x * 0.2f, io.DisplaySize.y * 0.25f);

    ImGui::SetNextWindowPos(ImVec2(15 + io.DisplaySize.x * 0.25f, 30), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowBgAlpha(0.35f);

    if (ImGui::Begin("Debug##dpi", nullptr,
        ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize))
    {
        
        //int gl_dw, gl_dh;
        
        //SDL_GL_GetDrawableSize(window, &gl_dw, &gl_dh);

        //int fb_w, fb_h;
        //SDL_GL_GetDrawableSize(window, &fb_w, &fb_h);

        ImGui::Text("---- Environment ----");

        if (PlatformManager::get()->is_mobile())
            ImGui::Text("Handheld:               TRUE");
        else
            ImGui::Text("Handheld:               FALSE");

        if (PlatformManager::get()->is_touch_device())
            ImGui::Text("Touchscreen:           TRUE");
        else
            ImGui::Text("Touchscreen:           FALSE");

        if (PlatformManager::get()->device_vertical())
            ImGui::Text("Vertical:                   TRUE");
        else
            ImGui::Text("Vertical:                   FALSE");

        ImGui::Text("FontSize                  %.1f", io.Fonts->Fonts[0]->FontSize);
        ImGui::Text("Touch Accuracy       %.2f", PlatformManager::get()->touch_accuracy());

        ImGui::Text("---- Scales ----");
        ImGui::Text("Character Rows       %.1f", io.DisplaySize.y / io.Fonts->Fonts[0]->FontSize);
        ImGui::Text("Character Cols       %.1f", io.DisplaySize.x / io.Fonts->Fonts[0]->FontSize);
        ImGui::Text("FontGlobalScale      %.2f", io.FontGlobalScale);
        ImGui::Text("FramebufferScale    %.3f x %.3f", io.DisplayFramebufferScale.x, io.DisplayFramebufferScale.y);
        ImGui::Text("---- Dimensions ----");
        ImGui::Text("Width                     %.1f inches", PlatformManager::get()->window_width_inches());
        ImGui::Text("Height                    %.1f inches", PlatformManager::get()->window_height_inches());
        ImGui::Text("---- DPI ----");
        ImGui::Text("DPI:                         %.1f", PlatformManager::get()->dpi());
        ImGui::Text("DPR:                        %.1f", PlatformManager::get()->dpr());
        ImGui::Text("DisplaySize:            %.1f x %.1f", io.DisplaySize.x, io.DisplaySize.y);
        ImGui::Text("WindowSize:           %d x %d",
            PlatformManager::get()->window_width(),
            PlatformManager::get()->window_height()
        );
        ImGui::Text("DrawableSize:         %d x %d",
            PlatformManager::get()->drawable_width(),
            PlatformManager::get()->drawable_height()
        );
        //ImGui::Text("DrawableSize (GL):   %d x %d", gl_dw, gl_dh);

        #if __EMSCRIPTEN__
        int canvas_w, canvas_h;
        double css_w, css_h;
        emscripten_get_canvas_element_size("#canvas", &canvas_w, &canvas_h);
        emscripten_get_element_css_size("#canvas", &css_w, &css_h);
        ImGui::Text("CanvasSize (bb):        %d x %d", canvas_w, canvas_h);
        ImGui::Text("CanvasSize (css):      %.1f x %.1f", css_w, css_h);
        #endif
    }
    ImGui::End();
}
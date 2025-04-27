#pragma once

#include <deque>
#include <string>
#include <imgui.h>

struct LogWindow
{
    std::deque<std::string> logLines;
    bool autoScroll = true;

    void Clear()
    {
        logLines.clear();
    }

    void Log(const char* fmt, ...)
    {
        char buffer[1024];

        va_list args;
        va_start(args, fmt);
        vsnprintf(buffer, sizeof(buffer), fmt, args);
        va_end(args);

        buffer[sizeof(buffer) - 1] = 0; // ensure null-termination

        if (logLines.size() >= 128)
            logLines.pop_front();
        logLines.emplace_back(buffer);
    }

    void Log(const std::string& message) {
        if (logLines.size() >= 128)
            logLines.pop_front();
        logLines.emplace_back(message);
    }

    void Draw(const char* title = "Log Window")
    {
        ImGuiIO& io = ImGui::GetIO();
        ImVec2 initSize(io.DisplaySize.x * 0.25f, io.DisplaySize.y * 1.0f);
        ImGui::SetNextWindowPos(ImVec2(10, 30), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(initSize, ImGuiCond_FirstUseEver);

        if (ImGui::Begin(title)) {
            ImGui::BeginChild("ScrollingRegion", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);
            for (const auto& line : logLines)
                ImGui::TextUnformatted(line.c_str());

            if (autoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
                ImGui::SetScrollHereY(1.0f); // scroll to bottom

            ImGui::EndChild();
        }
        ImGui::End();
    }
};
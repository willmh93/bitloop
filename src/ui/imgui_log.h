#pragma once

#include <deque>
#include <string>
#include <imgui.h>

struct ImDebugLog
{
    std::deque<std::string> logLines;
    bool autoScroll = true;

    void clear()
    {
        logLines.clear();
    }

    void vlog(const char* fmt, va_list ap)
    {
        char buffer[1024];
        vsnprintf(buffer, sizeof(buffer), fmt, ap); 
        if (logLines.size() >= 256)
            logLines.pop_front();
        logLines.emplace_back(buffer);
    }

    void log(const std::string& message) 
    {
        if (logLines.size() >= 256)
            logLines.pop_front();
        logLines.emplace_back(message);
    }

    void log(const char* fmt, ...)
    {
        va_list ap;
        va_start(ap, fmt);
        vlog(fmt, ap);        // forward
        va_end(ap);
    }

    void draw()
    {
        ImGui::BeginChild("ScrollingRegion", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);
        for (const auto& line : logLines)
            ImGui::TextUnformatted(line.c_str());

        if (autoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
            ImGui::SetScrollHereY(1.0f); // scroll to bottom

        ImGui::EndChild();
    }
};

//ImDebugLog* ImDebugLog::instance = nullptr;
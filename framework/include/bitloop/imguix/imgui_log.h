#pragma once

#include <deque>
#include <string>
#include <imgui.h>
#include "threads.h"

BL_BEGIN_NS

struct ImDebugLog
{
    std::deque<std::string> logLines;
    bool autoScroll = true;
    std::mutex log_mutex;

    void clear()
    {
        logLines.clear();
    }

    void vlog(const char* fmt, va_list ap)
    {
        std::lock_guard<std::mutex> lock(log_mutex);

        char buffer[1024];
        vsnprintf(buffer, sizeof(buffer), fmt, ap); 
        if (logLines.size() >= 256)
            logLines.pop_front();
        //OutputDebugStringA("Add Log Message\n");
        logLines.emplace_back(buffer);
    }

    void log(const std::string& message) 
    {
        std::lock_guard<std::mutex> lock(log_mutex);

        if (logLines.size() >= 256)
            logLines.pop_front();
        //OutputDebugStringA("Add Log Message\n");
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
        //OutputDebugStringA("Begin Drawing log\n");
        ImGui::BeginChild("ScrollingRegion", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);
        
        {
            std::lock_guard<std::mutex> lock(log_mutex);
            for (const auto& line : logLines)
                ImGui::TextUnformatted(line.c_str());
        }
        //OutputDebugStringA("End Drawing log\n");

        if (autoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
            ImGui::SetScrollHereY(1.0f); // scroll to bottom

        ImGui::EndChild();
    }
};

//ImDebugLog* ImDebugLog::instance = nullptr;

BL_END_NS
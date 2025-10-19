#pragma once

#include <deque>
#include <string>
#include <imgui.h>
#include "threads.h"
#include <mutex>

BL_BEGIN_NS

struct ImDebugLogMsg
{
    std::string txt;
    int count = 1;

    ImDebugLogMsg(std::string msg) : txt(msg) {}
    std::string getFullText() const
    {
        if (count == 1) return txt;
        return txt + "(x" + std::to_string(count) + ")";
    }
};

struct ImDebugLog
{
    std::deque<ImDebugLogMsg> logLines;
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

        if (logLines.size() && logLines.back().txt == buffer)
        {
            logLines.back().count++;
        }
        else
        {
            if (logLines.size() >= 256)
                logLines.pop_front();

            logLines.emplace_back(buffer);
        }
    }

    void log(const std::string& message) 
    {
        std::lock_guard<std::mutex> lock(log_mutex);

        if (logLines.size() >= 256)
            logLines.pop_front();

        if (logLines.size() && logLines.back().txt == message)
        {
            logLines.back().count++;
        }
        else
        {
            logLines.emplace_back(message);
        }
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
                ImGui::TextUnformatted(line.getFullText().c_str());
        }
        //OutputDebugStringA("End Drawing log\n");

        if (autoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
            ImGui::SetScrollHereY(1.0f); // scroll to bottom

        ImGui::EndChild();
    }
};

//ImDebugLog* ImDebugLog::instance = nullptr;

BL_END_NS

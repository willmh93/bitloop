#pragma once
#include <SDL2/SDL.h>
#include <vector>
#include "threads.h"

class WindowManager
{
    static WindowManager* singleton;

public:

    std::vector<SDL_Event> input_event_queue;
    std::mutex event_queue_mutex;

    static WindowManager* get()
    {
        if (singleton == nullptr)
            singleton = new WindowManager();
        return singleton;
    }

    void queueEvent(const SDL_Event& event); // Feed SDL event to event queue
    void queueData();                        // Feed Live data to shadow buffer (i.e. Queue it)

    void pollData();     // Process queued events
    void pollEvents();   // Processed queued data (*if* modified by ImGui inputs)

    ~WindowManager() {}
};

static WindowManager* MainWindow()
{
    return WindowManager::get();
}
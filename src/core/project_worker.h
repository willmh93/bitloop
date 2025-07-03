#pragma once
#include "threads.h"
#include <SDL3/SDL.h>

class ProjectBase;
class Canvas;
struct ImDebugLog;

enum ProjectCommandType
{
    PROJECT_SET,
    PROJECT_START,
    PROJECT_STOP,
    PROJECT_PAUSE
};

constexpr int CURRENT_PROJECT = -1;

extern ImDebugLog project_log;

struct ProjectCommandEvent
{
    ProjectCommandType type;
    int project_uid = CURRENT_PROJECT;
};

class ProjectBase;

class ProjectWorker
{
    static ProjectWorker* singleton;


    std::vector<SDL_Event> input_event_queue;
    std::mutex event_queue_mutex;

    std::thread worker_thread;

    ProjectBase* active_project = nullptr;

    std::vector<ProjectCommandEvent> project_command_queue;

    void _destroyActiveProject();

    void _onEvent(SDL_Event& e);

protected:

    SharedSync& shared_sync;

protected:

    friend class MainWindow;
    friend class ProjectBase;

    void draw();
    void populateAttributes();

public:

    [[nodiscard]] static ProjectWorker* instance() {
        return singleton;
    }

    ProjectWorker(SharedSync& _shared_sync) : shared_sync(_shared_sync) {
        singleton = this;
    }

    // ======== Thread Control ========
    void startWorker();
    void end();
    void worker_loop();

    // ======== Events / Data ========
    void handleProjectCommands(ProjectCommandEvent& e);

    void queueEvent(const SDL_Event& event); // Feed SDL event to event queue
    void pushDataToShadow();                        // Feed Live data to shadow buffer (i.e. Queue it)

    void pullDataFromShadow();     // Process queued events
    void pollEvents(bool discardBatch);   // Process queued data (*if* modified by ImGui inputs)

    // ======== Project Control ========
    [[nodiscard]] ProjectBase* getActiveProject() { return active_project; }

    void setActiveProject(int uid)  { project_command_queue.push_back({ PROJECT_SET,   uid }); }
    void startProject()             { project_command_queue.push_back({ PROJECT_START, CURRENT_PROJECT }); }
    void stopProject()              { project_command_queue.push_back({ PROJECT_STOP,  CURRENT_PROJECT }); }
    void pauseProject()             { project_command_queue.push_back({ PROJECT_PAUSE, CURRENT_PROJECT }); }
};
#pragma once
#include "threads.h"
#include <SDL2/SDL.h>

class ProjectBase;
class Canvas;
struct ImDebugLog;

enum ProjectControlEventType
{
    PROJECT_SET,
    PROJECT_START,
    PROJECT_STOP,
    PROJECT_PAUSE
};

constexpr int CURRENT_PROJECT = -1;

extern ImDebugLog project_log;

struct ProjectControlEvent
{
    ProjectControlEventType type;
    int project_uid = CURRENT_PROJECT;
};

class CProjectWorker
{
    static CProjectWorker* singleton;

    SharedSync& shared_sync;

    std::vector<SDL_Event> input_event_queue;
    std::mutex event_queue_mutex;

    std::thread worker_thread;

    ProjectBase* active_project = nullptr;

    std::vector<ProjectControlEvent> msg_queue;

    void _destroyActiveProject();

    void _onEvent(SDL_Event& e);

protected:

    friend class CMainWindow;

    void process();
    void draw();
    void populateAttributes();

public:

    static CProjectWorker* get() {
        return singleton;
    }

    CProjectWorker(SharedSync& _shared_sync) : shared_sync(_shared_sync) {
        singleton = this;
    }

    // ======== Thread Control ========
    void startWorker();
    void end();
    void worker_loop();

    // ======== Events / Data ========
    void handleProjectControlEvent(ProjectControlEvent& e);

    void queueEvent(const SDL_Event& event); // Feed SDL event to event queue
    void queueData();                        // Feed Live data to shadow buffer (i.e. Queue it)

    void pollData();     // Process queued events
    void pollEvents();   // Process queued data (*if* modified by ImGui inputs)

    void updateLiveAttributes();
    void updateShadowAttributes();

    // ======== Project Control ========
    ProjectBase* getActiveProject() { return active_project; }

    void setActiveProject(int uid)  { msg_queue.push_back({ PROJECT_SET,   uid }); }
    void startProject()             { msg_queue.push_back({ PROJECT_START, CURRENT_PROJECT }); }
    void stopProject()              { msg_queue.push_back({ PROJECT_STOP,  CURRENT_PROJECT }); }
    void pauseProject()             { msg_queue.push_back({ PROJECT_PAUSE, CURRENT_PROJECT }); }
};

static CProjectWorker* ProjectWorker()
{
    return CProjectWorker::get();
}
#pragma once
#include "threads.h"
#include <SDL3/SDL.h>

BL_BEGIN_NS

class ProjectBase;
class Canvas;
struct ImDebugLog;

enum struct ProjectCommandType
{
    PROJECT_SET,
    PROJECT_START,
    PROJECT_STOP,
    PROJECT_PAUSE
};

struct ProjectID
{
    static constexpr int CURRENT_PROJECT = -1;
    int uid;

    ProjectID(int _uid) : uid(_uid) {}
    operator int& () { return uid; }
};


extern ImDebugLog project_log;

struct ProjectCommandEvent
{
    ProjectCommandType type;
    ProjectID project_uid = ProjectID::CURRENT_PROJECT;
};

class ProjectBase;

class ProjectWorker
{
    static ProjectWorker* singleton;


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
    void drawOverlay();

public:
    std::vector<SDL_Event> input_event_queue;

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

    void pushDataToShadow();       // Feed Live data to shadow buffer (i.e. Queue it)
    void pullDataFromShadow();     // Process queued events

    void queueEvent(const SDL_Event& event); // Feed SDL event to event queue
    void pollEvents(bool discardBatch);      // Process queued data (*if* modified by ImGui inputs)

    // ======== Project Control ========
    [[nodiscard]] ProjectBase* getActiveProject() { return active_project; }

    void setActiveProject(int uid)  { project_command_queue.push_back({ ProjectCommandType::PROJECT_SET,   uid }); }
    void startProject()             { project_command_queue.push_back({ ProjectCommandType::PROJECT_START, ProjectID::CURRENT_PROJECT }); }
    void stopProject()              { project_command_queue.push_back({ ProjectCommandType::PROJECT_STOP,  ProjectID::CURRENT_PROJECT }); }
    void pauseProject()             { project_command_queue.push_back({ ProjectCommandType::PROJECT_PAUSE, ProjectID::CURRENT_PROJECT }); }
};

BL_END_NS

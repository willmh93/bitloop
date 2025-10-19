#pragma once
#include "threads.h"
#include <SDL3/SDL.h>

BL_BEGIN_NS

class ProjectBase;
class Canvas;
class CaptureManager;
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


class ProjectWorker
{
    static ProjectWorker* singleton;

    CaptureManager* capture_manager = nullptr; // Owned by MainWindow

    std::vector<ProjectCommandEvent> project_command_queue;
    std::mutex event_queue_mutex;
    std::thread worker_thread;

    ProjectBase* active_project = nullptr;

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

    std::vector<SDL_Event> input_event_queue;

    [[nodiscard]] static constexpr ProjectWorker* instance() {
        return singleton;
    }

    ProjectWorker(SharedSync& _shared_sync, CaptureManager* _capture_manager) :
        shared_sync(_shared_sync),
        capture_manager(_capture_manager)
    {
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

    std::mutex command_mutex;
    void addProjectCommand(ProjectCommandEvent e)
    {
        std::lock_guard<std::mutex> lock(command_mutex);
        project_command_queue.push_back(e);
    }

    void setActiveProject(int uid)  { addProjectCommand({ ProjectCommandType::PROJECT_SET,   uid }); }
    void startProject()             { addProjectCommand({ ProjectCommandType::PROJECT_START, ProjectID::CURRENT_PROJECT }); }
    void stopProject()              { addProjectCommand({ ProjectCommandType::PROJECT_STOP,  ProjectID::CURRENT_PROJECT }); }
    void pauseProject()             { addProjectCommand({ ProjectCommandType::PROJECT_PAUSE, ProjectID::CURRENT_PROJECT }); }
};

[[nodiscard]] constexpr ProjectWorker* project_worker()
{
    return ProjectWorker::instance();
}

BL_END_NS

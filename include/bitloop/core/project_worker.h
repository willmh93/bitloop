#pragma once
#include <bitloop/core/threads.h>
#include <bitloop/core/types.h>
#include <bitloop/core/snapshot_presets.h>

#include <SDL3/SDL.h>

BL_BEGIN_NS

class ProjectBase;
class Canvas;
class CaptureManager;
struct ImDebugLog;

enum struct ProjectCommandType
{
    PROJECT_SET,
    PROJECT_PLAY,
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

    ProjectBase* current_project = nullptr;

    void _destroyActiveProject();

    void _onEvent(SDL_Event& e);

protected:

    SharedSync& shared_sync;

protected:

    friend class MainWindow;
    friend class ProjectBase;

    void draw();
    void populateAttributes();
    void populateOverlay();

    void onEncodeFrame(EncodeFrame& data, int request_id, const CapturePreset& preset);

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
    ~ProjectWorker();

    // ======== Thread Control ========
    void startWorker();
    void worker_loop();

    // ======== Events / Data ========
    void handleProjectCommands(ProjectCommandEvent& e);

    void pushDataToShadow();               // Feed Live data to shadow buffer
    void pushDataToUnchangedShadowVars();  // Feed Live data to shadow buffer (IF shadow var is unchanged)
    void pullDataFromShadow();             // Process queued events

    void queueEvent(const SDL_Event& event); // Feed SDL event to event queue
    void pollEvents();                       // Process queued data (if modified by ImGui inputs)

    // ======== Project Control ========
    [[nodiscard]] ProjectBase* getCurrentProject() { return current_project; }
    [[nodiscard]] bool hasCurrentProject();

    std::mutex command_mutex;
    void addProjectCommand(ProjectCommandEvent e)
    {
        std::lock_guard<std::mutex> lock(command_mutex);
        project_command_queue.push_back(e);
    }

    void setActiveProject(int uid)  { addProjectCommand({ ProjectCommandType::PROJECT_SET,   uid }); }
    void startProject()             { addProjectCommand({ ProjectCommandType::PROJECT_PLAY, ProjectID::CURRENT_PROJECT }); }
    void stopProject()              { addProjectCommand({ ProjectCommandType::PROJECT_STOP,  ProjectID::CURRENT_PROJECT }); }
    void pauseProject()             { addProjectCommand({ ProjectCommandType::PROJECT_PAUSE, ProjectID::CURRENT_PROJECT }); }

    bool isSwitchingProject()
    {
        std::lock_guard<std::mutex> lock(command_mutex);
        for (auto& command : project_command_queue) {
            if (command.type == ProjectCommandType::PROJECT_SET)
                return true;
        }
        return false;
    }
};

[[nodiscard]] constexpr ProjectWorker* project_worker()
{
    return ProjectWorker::instance();
}

BL_END_NS

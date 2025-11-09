#pragma once

#include <vector>
#include <memory>
#include <string>

#include <bitloop/platform/platform.h>
#include <bitloop/imguix/imgui_custom.h>

#include "input.h"
#include "layout.h"

#include "types.h"
#include "event.h"
#include "var_buffer.h"

#ifdef SIM_BEG
#undef SIM_BEG
#endif
#ifdef SIM_END
#undef SIM_END
#endif
#define SIM_BEG  namespace SIM_NAME {
#define SIM_END  }
                             
BL_BEGIN_NS

class Viewport;
class ProjectBase;
class ProjectManager;
class Canvas;

struct ImDebugLog;

// Switch to "Input"
// Input.mouse
// Input.touch
// Input.touch.fingers
// Input.pointer (composite of mouse on finger 0)
// Or just use single:
// > Input.pointers[0].x
// > Input.pointers[0].y
// > Input.pointers[0].pressed
// Then use in viewport::process directly (always for consistency/predictability?)



/// ======= Input ==========
/*
struct SceneFingerInfo : FingerInfo
{
    Viewport* viewport = nullptr;
    double client_x = 0;
    double client_y = 0;
    double stage_x = 0;
    double stage_y = 0;
    double world_x = 0;
    double world_y = 0;
    int scroll_delta = 0;
    bool pressed = false;

    SceneFingerInfo(const FingerInfo& f) : FingerInfo(f)
    {}
};

struct TouchState
{
    //PointerEvent event; // The actual event which updated this state
    std::vector<SceneFingerInfo> fingers; // "Current" state of all fingers during this state

    //TouchState(const Event& e, const std::vector<FingerInfo>& fingers);
    //TouchState(const std::vector<FingerInfo>& fingers);

    const SceneFingerInfo* finger(int i=0) const
    {
        if (i < (int)fingers.size())
            return &fingers[i];
        return nullptr;
    }
};

struct ScrollState
{
    PointerEvent event; // The event which updated this state

    int scroll_delta = 0;
};

struct KeyboardState
{
    KeyEvent event; // The event which updated this state
};

struct TouchInput : TouchState
{
    std::vector<TouchState> updates;
    //TouchState present;

    //TouchState

    //const TouchState& now()const  {
    //    if (updates.size())
    //        return updates.back();
    //
    //    static SDL_Event dummy_sdl;
    //    static Event dummy(dummy_sdl);
    //    static std::vector<FingerInfo> dummy_fingers;
    //    static TouchState dummy_state(dummy, dummy_fingers);
    //    return dummy_state;
    //}

    void push(const TouchState& s) {
        updates.push_back(s);
        static_cast<TouchState&>(*this) = s;
    }

    void clear() {
        updates.clear();
    }
};

struct Input
{
    TouchInput touch;
    //InputStateQueue<KeyboardState>  keyboard;

    void clear()
    {
        touch.clear();
        //keyboard.clear();
    }

    void addEvent(const Event& e, const std::vector<FingerInfo>& fingers)
    {
        if (e.isPointerEvent())
        {
            UNUSED(fingers);
            //TouchState s(fingers);
            //touch.push(s);
        }
    }
};
*/
/// =================

struct ProjectInfo
{
    enum State { INACTIVE, ACTIVE, RECORDING };

    std::string name;
    std::vector<std::string> path;
    ProjectCreatorFunc creator;
    int sim_uid;
    State state;

    ProjectInfo(
        std::vector<std::string> path,
        std::string name="",
        ProjectCreatorFunc creator = nullptr,
        int sim_uid = -100,
        State state = State::INACTIVE
    )
        : name(name), path(path), creator(creator), sim_uid(sim_uid), state(state)
    {}
};

struct ProjectInfoNode
{
    std::shared_ptr<ProjectInfo> project_info = nullptr;
    std::vector<ProjectInfoNode> children;
    std::string name;

    ProjectInfoNode(std::string node_name) : name(node_name) {}
    ProjectInfoNode(std::shared_ptr<ProjectInfo> project)
    {
        project_info = project;
        name = project->path.back();
    }
};

class ProjectBase
{
    Layout viewports;
    int scene_counter = 0;
    int sim_uid = -1;

    double dt_projectProcess = 0;
    double dt_frameProcess = 0;

    const int splitter_thickness = 6;// 6;

    std::chrono::steady_clock::time_point last_frame_time 
        = std::chrono::steady_clock::now();

    // Keep track of focused/hovered project viewports
    Viewport* ctx_focused = nullptr;
    Viewport* ctx_hovered = nullptr;

protected:

    friend class ProjectWorker;
    friend class Layout;
    friend class SceneBase;
    friend class Viewport;

    Canvas* canvas = nullptr;
    ImDebugLog* project_log = nullptr;

    // ----------- States -----------
    bool started = false;
    bool paused = false;
    bool done_single_process = false;

    void configure(int sim_uid, Canvas* canvas, ImDebugLog* project_log);

    void updateViewportRects();

    // -------- Populating Project/Scene ImGui attributes --------
    void _populateAllAttributes();
    void _populateOverlay();
    virtual void _projectAttributes() {}

    // -------- Data Buffers --------
    bool has_var_buffer = false;

    virtual void updateProjectLiveBuffer() {}
    virtual void updateProjectShadowBuffer() {}

    virtual void updateLiveBuffers();
    virtual void updateShadowBuffers();
    virtual bool changedLive();
    virtual bool changedShadow();
    virtual void markLiveValues();
    virtual void markShadowValues();
    

    // ---- Project Management ----
    void _projectPrepare();
    void _projectStart();
    void _projectStop();
    void _projectPause();
    void _projectDestroy();
    void _projectProcess();
    void _projectDraw();

    std::vector<FingerInfo> pressed_fingers;
    void _clearEventQueue();
    void _onEvent(SDL_Event& e);

public:

    MouseInfo mouse;

    virtual ~ProjectBase() = default;
    
    // BasicProject Factory methods
    [[nodiscard]] static std::vector<std::shared_ptr<ProjectInfo>>& projectInfoList()
    {
        static std::vector<std::shared_ptr<ProjectInfo>> info_list;
        return info_list;
    }

    [[nodiscard]] static ProjectInfoNode& projectTreeRootInfo()
    {
        static ProjectInfoNode root("root");
        return root;
    }

    [[nodiscard]] static std::shared_ptr<ProjectInfo> findProjectInfo(int sim_uid)
    {
        for (auto& info : projectInfoList())
        {
            if (info->sim_uid == sim_uid)
                return info;
        }
        return nullptr;
    }

    [[nodiscard]] static std::shared_ptr<ProjectInfo> findProjectInfo(const char *name)
    {
        for (auto& info : projectInfoList())
        {
            if (info->path.back() == name)
                return info;
        }
        return nullptr;
    }

    static inline int factory_sim_index = 0;

    template<typename T>
    static std::shared_ptr<ProjectInfo> createProjectFactoryInfo(std::string name)
    {
        auto project_info = std::make_shared<ProjectInfo>(T::info());
        project_info->creator = []() -> ProjectBase* { return new T(); };
        project_info->name = name;
        project_info->sim_uid = ProjectBase::factory_sim_index++;

        return project_info;
    }

    static void addProjectFactoryInfo(std::shared_ptr<ProjectInfo> project_info)
    {
        std::vector<std::shared_ptr<ProjectInfo>>& project_list = projectInfoList();

        project_list.push_back(project_info);

        ProjectInfoNode* current = &projectTreeRootInfo();

        // Traverse tree and insert info in correct nested category
        for (size_t i = 0; i < project_info->path.size(); i++)
        {
            const std::string& insert_name = project_info->path[i];
            bool is_leaf = (i == (project_info->path.size() - 1));

            if (is_leaf)
            {
                current->children.push_back(ProjectInfoNode(project_info));
                break;
            }
            else
            {
                // Does category already exist?
                ProjectInfoNode* found_existing = nullptr;
                for (ProjectInfoNode& existing_node : current->children)
                {
                    if (existing_node.name == insert_name)
                    {
                        // Yes
                        found_existing = &existing_node;
                        break;
                    }
                }
                if (found_existing)
                {
                    current = found_existing;
                }
                else
                {
                    current->children.push_back(ProjectInfoNode(insert_name));
                    current = &current->children.back();
                }
            }
        }
    }

    [[nodiscard]] std::shared_ptr<ProjectInfo> getProjectInfo()
    {
        return findProjectInfo(sim_uid);
    }

    void setProjectInfoState(ProjectInfo::State state);

    // Shared Scene creators

    template<typename SceneType>
    [[nodiscard]] SceneType* create()
    {
        auto config = std::make_shared<typename SceneType::Config>();
        SceneType* scene;

        if constexpr (std::is_constructible_v<SceneType, typename SceneType::Config&>)
        {
            scene = new SceneType(*config);
            scene->active_config = config;

        }
        else
            scene = new SceneType();

        scene->project = this;
        scene->scene_index = scene_counter++;

        return scene;
    }

    template<typename SceneType> [[nodiscard]] SceneType* create(typename SceneType::Config config)
    {
        auto config_ptr = std::make_shared<typename SceneType::Config>(config);

        SceneType* scene = new SceneType(*config_ptr);
        scene->active_config = config_ptr;
        scene->scene_index = scene_counter++;
        scene->project = this;

        return scene;
    }

    template<typename SceneType>
    [[nodiscard]] SceneType* create(std::shared_ptr<typename SceneType::Config> config)
    {
        if (!config)
            throw "Launch Config wasn't created";

        SceneType* scene = new SceneType(*config);
        scene->active_config = config;
        scene->scene_index = scene_counter++;
        scene->project = this;

        return scene;
    }

    template<typename SceneType>
    [[nodiscard]] std::shared_ptr<SimSceneList> create(int count)
    {
        auto ret = std::make_shared<SimSceneList>();
        for (int i = 0; i < count; i++)
        {
            SceneType* scene = create<SceneType>();
            scene->project = this;

            ret->push_back(scene);
        }
        return ret;
    }

    template<typename SceneType>
    [[nodiscard]] std::shared_ptr<SimSceneList> create(int count, typename SceneType::Config config)
    {
        auto ret = std::make_shared<SimSceneList>();
        for (int i = 0; i < count; i++)
        {
            SceneType* scene = create<SceneType>(config);
            scene->project = this;

            ret->push_back(scene);
        }
        return ret;
    }

    Layout& newLayout();
    Layout& newLayout(int _viewports_x, int _viewports_y);
    [[nodiscard]] Layout& currentLayout()
    {
        return viewports;
    }

    [[nodiscard]] bool isRecording() const;

    virtual std::vector<std::string> categorize()
    {
        return { "New Projects", "Project" };
    }

    virtual void projectAttributes() {}
    virtual void projectPrepare(Layout& layout) = 0;
    virtual void projectStart() {}
    virtual void projectStop() {}
    virtual void projectDestroy() {}

    virtual void onEvent(Event&) {}

    void pullDataFromShadow();
    void pollEvents();

    void logMessage(const char* fmt, ...);
    void logClear();
};

template<typename ProjectType>
class Project : public ProjectBase, public VarBuffer<ProjectType>
{
    friend class ProjectBase;

public:

    using Interface = VarBufferInterface<ProjectType>;
    Interface* ui = nullptr;

    Project() : ProjectBase()
    {
        has_var_buffer = true;
        ui = new ProjectType::UI(static_cast<const ProjectType*>(this));
    }

    ~Project()
    {
        delete ui;
    }

    virtual void _projectAttributes() override
    {
        ui->sidebar();
    }

protected:

    void updateLiveBuffers() override final
    {
        ProjectBase::updateLiveBuffers();
        VarBuffer<ProjectType>::updateLive();
        VarBuffer<ProjectType>::runScheduledCalls();
    }
    void updateShadowBuffers() override final
    {
        ProjectBase::updateShadowBuffers();
        VarBuffer<ProjectType>::updateShadow();
    }
    void markLiveValues() override final
    {
        ProjectBase::markLiveValues();
        VarBuffer<ProjectType>::markLiveValue();
    }
    void markShadowValues() override final
    {
        ProjectBase::markShadowValues();
        VarBuffer<ProjectType>::markShadowValue();
    }
    bool changedLive() override final
    {
        if (ProjectBase::changedLive()) return true;
        return VarBuffer<ProjectType>::liveChanged();
    }
    bool changedShadow() override final
    {
        if (ProjectBase::changedShadow()) return true;
        return VarBuffer<ProjectType>::shadowChanged();
    }
};

typedef ProjectBase BasicProject;


BL_END_NS

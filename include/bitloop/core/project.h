#pragma once

#include <vector>
#include <memory>
#include <string>

#include <bitloop/util/timer.h>
#include <bitloop/platform/platform.h>
#include <bitloop/core/interface_model.h>
#include <bitloop/core/snapshot_presets.h>
#include <bitloop/core/capture_manager.h>
#include <bitloop/imguix/imguix.h>

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

class ProjectBase;
using ProjectCreatorFunc = std::function<ProjectBase*()>;

struct ProjectInfo
{
    std::string name;
    std::vector<std::string> path;
    const char* dev_root; // nullptr if not BITLOOP_DEV_MODE

    ProjectCreatorFunc creator;
    int sim_uid;

    ProjectInfo(
        std::vector<std::string> path,
        std::string name="",
        ProjectCreatorFunc creator = nullptr,
        int sim_uid = -100
    )
        : name(name), path(path), creator(creator), sim_uid(sim_uid)
    {}
};

struct ProjectInfoNode
{
    std::shared_ptr<ProjectInfo> project_info = nullptr;
    std::vector<ProjectInfoNode> children;
    std::string name;
    int uid = -1;

    ProjectInfoNode(std::string node_name) : name(node_name) {}
    ProjectInfoNode(std::shared_ptr<ProjectInfo> project)
    {
        project_info = project;
        name = project->path.back();
    }

    int count_nodes() const
    {
        int count = 1u;
        for (const auto& child : children)
            count += child.count_nodes();
        return count;
    }
};

class ProjectBase
{
    Layout viewports;
    int scene_counter = 0;
    int sim_uid = -1;

    double dt_projectProcess = 0;
    double dt_frameProcess = 0;

    const int splitter_thickness = 6;

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

    // ----- states -----
    bool started = false;
    bool paused = false;
    bool done_single_process = false;

    void configure(int sim_uid, Canvas* canvas, ImDebugLog* project_log);

    void updateViewportRects();

    // ----- populating Project/Scene ImGui attributes -----
    void _populateAllAttributes();
    virtual void _populateOverlay();
    virtual void _projectAttributes() {}

    virtual void _initGUI();
    virtual void _destroyGUI();
    
    // ----- Project management (internal, invokes public virtual methods) -----
    void _projectPrepare();
    void _projectStart();
    void _projectStop();
    void _projectPause();
    void _projectResume();
    void _projectDestroy();
    void _projectProcess();
    void _projectDraw();

    // ----- capturing -----

    void _onEncodeFrame(EncodeFrame& data, int request_id, const CapturePreset& preset);

    // ----- input -----
    std::vector<FingerInfo> pressed_fingers;
    void _clearEventQueue();
    void _onEvent(SDL_Event& e);


    // ----- data buffers -----
    virtual void updateProjectLiveBuffer() {}
    virtual void updateProjectShadowBuffer() {}

    virtual void updateLiveBuffers();
    virtual void updateShadowBuffers();
    virtual void updateUnchangedShadowVars();
    virtual bool changedLive();
    virtual bool changedShadow();
    virtual void markLiveValues();
    virtual void markShadowValues();
    virtual void invokeScheduledCalls();

public:

    static ProjectBase* activeProject();

    // todo: improve access
    MouseInfo mouse;

    virtual ~ProjectBase() = default;

    virtual InterfaceModel* getInterfaceModel() = 0;
    
    /// ----- Project factory -----

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
    static std::shared_ptr<ProjectInfo> createProjectFactoryInfo(std::string name, const char* dev_root = nullptr)
    {
        auto project_info = std::make_shared<ProjectInfo>(T::info());
        project_info->creator = []() -> ProjectBase* { return new T(); };
        project_info->name = name;
        project_info->sim_uid = ProjectBase::factory_sim_index++;
        project_info->dev_root = dev_root;
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
                current->children.back().uid = current->count_nodes();
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
                    current->children.back().uid = current->count_nodes();

                    current = &current->children.back();
                }
            }
        }
    }

    [[nodiscard]] std::shared_ptr<ProjectInfo> getProjectInfo() const
    {
        return findProjectInfo(sim_uid);
    }

    /// ----- Scene factory -----

    // create Scene with default config
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

    // create Scene with a provided initial config
    template<typename SceneType>
    [[nodiscard]] SceneType* create(typename SceneType::Config config)
    {
        auto config_ptr = std::make_shared<typename SceneType::Config>(config);

        SceneType* scene = new SceneType(*config_ptr);
        scene->active_config = config_ptr;
        scene->scene_index = scene_counter++;
        scene->project = this;
        return scene;
    }

    // overload to support using active_config from an existing Scene
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

    /// ----- layout -----

    Layout& newLayout();
    Layout& newLayout(int _viewports_x, int _viewports_y);
    [[nodiscard]] Layout& currentLayout()
    {
        return viewports;
    }

    /// ----- states -----

    [[nodiscard]] bool isPaused() const { return paused; }
    [[nodiscard]] bool isActive() const { return started; } // in "started/paused" state (but not stopped)

    #ifdef BITLOOP_DEV_MODE
    [[nodiscard]] std::string proj_path(std::string_view virtual_path) const
    {
        std::filesystem::path p = getProjectInfo()->dev_root;

        if (!virtual_path.empty() && virtual_path.front() == '/')
            virtual_path.remove_prefix(1); // trim leading '/'

        p /= virtual_path; // join
        p = p.lexically_normal(); // clean up
        return p.string();
    }
    #endif

    /// TODO: add IDBFS support for virtual file system on web?
    //
    // root_path("/path/to/file")
    // - useful to modify source data during development, but still behaves in release builds where no project folder exist
    // - for web builds, files are read-only
    // 
    // If DEFINED     (BITLOOP_DEV_MODE):  "root" = project folder
    // If NOT DEFINED (BITLOOP_DEV_MODE):  "root" = exe folder
    //
    [[nodiscard]] std::string root_path(std::string_view virtual_path) const
    {
        #ifdef BITLOOP_DEV_MODE
        return proj_path(virtual_path);
        #else
        return platform()->path(virtual_path);
        #endif
    }

    /// ----- overridable virtual methods -----

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

    /// ----- logging -----

    void logMessage(const char* fmt, ...);
    void logClear();
};

class BasicProject : public ProjectBase, public DirectInterfaceModel
{
    void _projectAttributes() override final
    {
        sidebar();
    }
    void _populateOverlay() override final
    {
        overlay();
    }

    InterfaceModel* getInterfaceModel() override final
    {
        return static_cast<InterfaceModel*>(this);
    }
};

template<typename ProjectType>
class Project : public ProjectBase, public VarBuffer<ProjectType>
{
    friend class ProjectBase;

public:

    using BufferedInterfaceModel = DoubleBufferedInterfaceModel<ProjectType>;
    BufferedInterfaceModel* ui = nullptr;

    Project() : ProjectBase() {}
    ~Project()
    {
        // should already be destroyed in _destroyGUI on GUI thread
        assert(ui == nullptr);
        if (ui) delete ui;
    }

protected:

    InterfaceModel* getInterfaceModel() override final
    {
        return static_cast<InterfaceModel*>(ui);
    }

    auto getUI() const noexcept
    {
        return static_cast<const ProjectType::UI*>(ui);
    }

    void _initGUI() override final
    {
        if (!ui)
        {
            // safer to initialize UI on same thread as UI populate
            ui = new ProjectType::UI(static_cast<const ProjectType*>(this));
            ui->init();
        }

        ProjectBase::_initGUI();
    }

    void _destroyGUI() override final
    {
        if (ui)
        {
            ui->destroy();
            delete ui;
            ui = nullptr;
        }

        ProjectBase::_destroyGUI();
    }

    void _projectAttributes() override final
    {
        ui->sidebar();
    }

    void _populateOverlay() override final
    {
        ui->overlay();
    }

    void updateLiveBuffers() override final
    {
        ProjectBase::updateLiveBuffers(); // updates scenes
        VarBuffer<ProjectType>::updateLive();
    }
    void updateShadowBuffers() override final
    {
        ProjectBase::updateShadowBuffers(); // updates scenes
        VarBuffer<ProjectType>::updateShadow();
    }
    void markLiveValues() override final
    {
        ProjectBase::markLiveValues(); // marks scenes
        VarBuffer<ProjectType>::markLiveValue();
    }
    void markShadowValues() override final
    {
        ProjectBase::markShadowValues(); // marks scenes
        VarBuffer<ProjectType>::markShadowValue();
    }
    bool changedLive() override final
    {
        if (ProjectBase::changedLive()) return true; // checks scenes
        return VarBuffer<ProjectType>::liveChanged();
    }
    bool changedShadow() override final
    {
        if (ProjectBase::changedShadow()) return true; // checks scenes
        return VarBuffer<ProjectType>::shadowChanged();
    }

    void updateUnchangedShadowVars() override final
    {
        ProjectBase::updateUnchangedShadowVars(); // updates scenes
        VarBuffer<ProjectType>::updateUnchangedShadowVars();
    }

    void invokeScheduledCalls() override final
    {
        ProjectBase::invokeScheduledCalls(); // invoke scheduled calls for all scenes
        VarBuffer<ProjectType>::invokeScheduledCalls();
    }
};



BL_END_NS

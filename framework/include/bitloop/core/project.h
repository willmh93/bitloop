#pragma once

#include <vector>
#include <memory>
#include <string>
#include <string_view>
#include <sstream>
#include <functional>
#include <type_traits>
#include <random>

#include <bitloop/util/text_util.h>
#include <bitloop/util/json.h>
#include <bitloop/util/change_tracker.h>
#include <bitloop/nanovgx/nano_canvas.h>
#include <bitloop/imguix/imgui_custom.h>
#include <bitloop/platform/platform.h>

#include "types.h"
#include "event.h"
#include "camera.h"
#include "project_worker.h"
#include "var_buffer.h"

using std::max;
using std::min;

// Provide macros for easy BasicProject registration
template<typename... Ts>
std::vector<std::string> VectorizeArgs(Ts&&... args) { return { std::forward<Ts>(args)... }; }

/*inline void attach_parent_console()
{
    if (!AttachConsole(ATTACH_PARENT_PROCESS))
        AllocConsole();

    // Route stdio streams to the console
    FILE* f;
    freopen_s(&f, "CONOUT$", "w", stdout);
    freopen_s(&f, "CONOUT$", "w", stderr);
}*/

#ifdef SIM_BEG
#undef SIM_BEG
#endif
#ifdef SIM_END
#undef SIM_END
#endif
#define SIM_BEG          namespace SIM_NAME {
#define SIM_END          }
                             
BL_BEGIN_NS

class Layout;
class Viewport;
class ProjectBase;
class ProjectManager;
struct ImDebugLog;

struct MouseInfo
{
    Viewport* viewport = nullptr;
    double client_x = 0;
    double client_y = 0;
    double stage_x = 0;
    double stage_y = 0;
    double world_x = 0;
    double world_y = 0;
    int scroll_delta = 0;
};

struct ProjectInfo
{
    enum State { INACTIVE, ACTIVE, RECORDING };

    std::vector<std::string> path;
    ProjectCreatorFunc creator;
    int sim_uid;
    State state;

    ProjectInfo(
        std::vector<std::string> path,
        ProjectCreatorFunc creator = nullptr,
        int sim_uid = -100,
        State state = State::INACTIVE
    )
        : path(path), creator(creator), sim_uid(sim_uid), state(state)
    {}
};

class SceneBase : public ChangeTracker
{
    mutable std::mt19937 gen;

    int dt_sceneProcess = 0;

    mutable std::vector<Math::MovingAverage::MA> dt_scene_ma_list;
    mutable std::vector<Math::MovingAverage::MA> dt_project_ma_list;
    //mutable std::vector<Math::MovingAverage::MA> dt_project_draw_ma_list;

    mutable size_t dt_call_index = 0;
    mutable size_t dt_process_call_index = 0;
    //size_t dt_draw_call_index = 0;

protected:

    friend class Viewport;
    friend class ProjectBase;

    ProjectBase* project = nullptr;

    int scene_index = -1;
    std::vector<Viewport*> mounted_to_viewports;

    // Mounting to/from viewport
    void registerMount(Viewport* viewport);
    void registerUnmount(Viewport* viewport);

    //
    bool has_var_buffer = false;
    virtual void _sceneAttributes() {}

    // In case Scene uses double buffer
    virtual void updateLiveBuffers() {}
    virtual void updateShadowBuffers() {}
    virtual bool changedLive() { return false; }
    virtual bool changedShadow() { return false; }
    virtual void markLiveValues() {}
    virtual void markShadowValues() {}

    //
    
    void _onEvent(Event e) 
    {
        onEvent(e);

        switch (e.type())
        {
        case SDL_EVENT_FINGER_DOWN:       onPointerDown(PointerEvent(e));  break;
        case SDL_EVENT_MOUSE_BUTTON_DOWN: onPointerDown(PointerEvent(e));  break;
        case SDL_EVENT_FINGER_UP:         onPointerUp(PointerEvent(e));    break;
        case SDL_EVENT_MOUSE_BUTTON_UP:   onPointerUp(PointerEvent(e));    break;
        case SDL_EVENT_FINGER_MOTION:     onPointerMove(PointerEvent(e));  break;
        case SDL_EVENT_MOUSE_MOTION:      onPointerMove(PointerEvent(e));  break;
        case SDL_EVENT_MOUSE_WHEEL:       onWheel(PointerEvent(e));        break;

        case SDL_EVENT_KEY_DOWN:          onKeyDown(KeyEvent(e));          break;
        case SDL_EVENT_KEY_UP:            onKeyUp(KeyEvent(e));            break;
        }
    }

public:

    std::shared_ptr<void> temporary_environment;

    Camera* camera = nullptr;
    MouseInfo* mouse = nullptr;

    struct Config {};

    SceneBase() : gen(std::random_device{}()) {}
    virtual ~SceneBase() = default;

    void mountTo(Viewport* viewport);
    void mountTo(Layout& viewports);
    void mountToAll(Layout& viewports);

    virtual void sceneStart() {}
    virtual void sceneMounted(Viewport*) {}
    virtual void sceneStop() {}
    virtual void sceneDestroy() {}
    virtual void sceneProcess() {}
    virtual void viewportProcess(Viewport*, double) {}
    virtual void viewportDraw(Viewport*) const = 0;

    virtual void onEvent(Event e) { (void)e; }

    void pollEvents();
    void pullDataFromShadow();

    virtual void onPointerEvent(PointerEvent) {}
    virtual void onPointerDown(PointerEvent) {}
    virtual void onPointerUp(PointerEvent) {}
    virtual void onPointerMove(PointerEvent) {}
    virtual void onWheel(PointerEvent) {}
    virtual void onKeyDown(KeyEvent) {}
    virtual void onKeyUp(KeyEvent) {}

    void handleWorldNavigation(Event e, bool single_touch_pan);

    virtual std::string name() const { return "Scene"; }
    [[nodiscard]] int sceneIndex() const { return scene_index; }

    [[nodiscard]] double frame_dt(int average_samples = 1) const;
    [[nodiscard]] double scene_dt(int average_samples = 1) const;
    [[nodiscard]] double project_dt(int average_samples = 1) const;

    [[nodiscard]] double fps(int average_samples = 1) const { return 1000.0 / frame_dt(average_samples); }

    ///FRect combinedViewportsRect()
    ///{
    ///    FRect ret{};
    ///    for (Viewport* ctx : viewports)
    ///    {
    ///        ctx->viewportRect();
    ///    }
    ///}

    [[nodiscard]] double random(double min = 0, double max = 1) const
    {
        std::uniform_real_distribution<> dist(min, max);
        return dist(gen);
    }

    [[nodiscard]] DVec2 Offset(double stage_offX, double stage_offY) const
    {
        return camera->stageToWorldOffset(DVec2{ stage_offX, stage_offY });
    }

    void logMessage(std::string_view, ...);
    void logClear();
};

struct Message
{
    int type;
    std::any data;
    Message(int type, std::any data) : type(type), data(std::move(data)) {}
};

template<DerivedFromVarBuffer VarBufferType>
class Scene : public SceneBase, public DoubleBuffer<VarBufferType>
{
    friend class ProjectBase;

    Thread::ConcurrentQueue<Message> msg_queue;

public:

    Scene() : SceneBase(), DoubleBuffer<VarBufferType>()
    {
        has_var_buffer = true;
    }

protected:

    virtual void _sceneAttributes() override 
    {
        DoubleBuffer<VarBufferType>::shadow_attributes.populateUI();
    }

private:

    void updateLiveBuffers() override final   { DoubleBuffer<VarBufferType>::updateLiveBuffer(); }
    void updateShadowBuffers() override final { DoubleBuffer<VarBufferType>::updateShadowBuffer(); }
    void markLiveValues() override final      { DoubleBuffer<VarBufferType>::markLiveValues();   }
    void markShadowValues() override final    { DoubleBuffer<VarBufferType>::markShadowValues(); }
    bool changedLive() override final         { return DoubleBuffer<VarBufferType>::changedLive(); }
    bool changedShadow() override final       { return DoubleBuffer<VarBufferType>::changedShadow(); }
};

class Viewport : public Painter
{
    std::string print_text;
    std::stringstream print_stream;

protected:

    friend class ProjectBase;
    friend class SceneBase;
    friend class Layout;
    friend class Camera;

    int viewport_index;
    int viewport_grid_x;
    int viewport_grid_y;

    Layout* layout = nullptr;
    SceneBase* scene = nullptr;

    double x = 0;
    double y = 0;
    double w = 0;
    double h = 0;
    double old_w = 0;
    double old_h = 0;
    bool just_resized = false;

public:
    

    Viewport(
        Layout* layout,
        Canvas *canvas,
        int viewport_index,
        int grid_x,
        int grid_y
    );

    ~Viewport();

    void draw();

    [[nodiscard]] int viewportIndex() { return viewport_index; }
    [[nodiscard]] int viewportGridX() { return viewport_grid_x; }
    [[nodiscard]] int viewportGridY() { return viewport_grid_y; }

    [[nodiscard]] double posX() { return x; }
    [[nodiscard]] double posY() { return y; }
    [[nodiscard]] double width() { return w; }
    [[nodiscard]] double height() { return h; }
    [[nodiscard]] DVec2 size() { return DVec2(w, h); }
    [[nodiscard]] DRect viewportRect() { return DRect(x, y, x + w, y + h); }
    [[nodiscard]] DQuad worldQuad() { return camera.toWorldQuad(0, 0, w, h); }
    [[nodiscard]] DVec2 worldSize() { return camera.stageToWorldOffset(w, h); }
    //[[nodiscard]] DAngledRect worldRect() { return camera.toWorldRect(DAngledRect(width/2, height/2, width, height, 0.0)); }

    template<typename T>
    T* mountScene(T* _sim)
    {
        blPrint() << "Mounting existing scene to Viewport: " << viewport_index;
        scene = _sim;
        scene->registerMount(this);
        return _sim;
    }

    // Viewport-specific draw helpers (i.e. size of viewport needed)
    void printTouchInfo();

    std::stringstream& print() {
        return print_stream;
    }
};


struct SimSceneList : public std::vector<SceneBase*>
{
    void mountTo(Layout& viewports);
};

class Layout
{
    std::vector<Viewport*> viewports;

protected:

    friend class ProjectBase;
    friend class SceneBase;

    // If 0, viewports expand in that direction. If both 0, expand whole grid.
    int targ_viewports_x = 0;
    int targ_viewports_y = 0;
    int cols = 0;
    int rows = 0;

    std::vector<SceneBase*> all_scenes;
    ProjectBase* project = nullptr;

public:

    using iterator = typename std::vector<Viewport*>::iterator;
    using const_iterator = typename std::vector<Viewport*>::const_iterator;

    ~Layout()
    {
        // Only invoked when you switch project
        clear();
    }

    const std::vector<SceneBase*>& scenes() {
        return all_scenes;
    }

    void expandCheck(size_t count);
    void add(int _viewport_index, int _grid_x, int _grid_y);
    void resize(size_t viewport_count);
    void clear()
    {
        // layout freed each time you call setLayout
        for (Viewport* p : viewports) delete p;
        viewports.clear();
    }

    void setSize(int _targ_viewports_x, int _targ_viewports_y)
    {
        this->targ_viewports_x = _targ_viewports_x;
        this->targ_viewports_y = _targ_viewports_y;
    }


    [[nodiscard]] Viewport* operator[](size_t i) { expandCheck(i + 1); return viewports[i]; }
    Layout&   operator<<(SceneBase* scene) { scene->mountTo(*this); return *this; }
    Layout&   operator<<(std::shared_ptr<SimSceneList> scenes) { 
        for (SceneBase *scene : *scenes)
            scene->mountTo(*this);
        return *this;
    }

    [[nodiscard]] iterator begin() { return viewports.begin(); }
    [[nodiscard]] iterator end() { return viewports.end(); }

    [[nodiscard]] const_iterator begin() const { return viewports.begin(); }
    [[nodiscard]] const_iterator end() const { return viewports.end(); }

    [[nodiscard]] int count() const {
        return static_cast<int>(viewports.size());
    }
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

    int dt_projectProcess = 0;
    double dt_frameProcess = 0;

    std::chrono::steady_clock::time_point last_frame_time 
        = std::chrono::steady_clock::now();

    Viewport* ctx_focused = nullptr;
    Viewport* ctx_hovered = nullptr;

protected:

    friend class ProjectWorker;
    friend class Layout;
    friend class SceneBase;

    Canvas* canvas = nullptr;
    ImDebugLog* project_log = nullptr;

    // ----------- States -----------
    bool started = false;
    bool paused = false;
    bool done_single_process = false;

    void configure(int sim_uid, Canvas* canvas, ImDebugLog* project_log);

    [[nodiscard]] DVec2 surfaceSize(); // Dimensions of canvas (or FBO if recording)
    void updateViewportRects();

    // -------- Data Buffers --------
    bool has_var_buffer = false;
    void _populateAllAttributes();
    virtual void _projectAttributes() {}
    virtual void updateProjectLiveBuffer() {}
    virtual void updateProjectShadowBuffer() {}

    // Exposed methods for updating project (and contained scenes)
    // live/shadow buffers from project_worker.
    // 
    // Even if this is a BasicProject (no double buffer), Scenes might
    // still inherit DoubleBuffer if they inherit DoubleBuffer

    virtual void updateLiveBuffers()
    {
        for (SceneBase* scene : viewports.all_scenes)
            scene->updateLiveBuffers();
    }
    virtual void updateShadowBuffers()
    {
        for (SceneBase* scene : viewports.all_scenes)
            scene->updateShadowBuffers();
    }

    virtual bool changedLive()
    {
        for (SceneBase* scene : viewports.all_scenes) {
            if (scene->changedLive())
                return true;
        }
        return false;
    }

    virtual bool changedShadow()
    {
        for (SceneBase* scene : viewports.all_scenes) {
            if (scene->changedShadow())
                return true;
        }
        return false;
    }

    virtual void markLiveValues()
    {
        for (SceneBase* scene : viewports.all_scenes)
            scene->markLiveValues();
    }

    virtual void markShadowValues()
    {
        for (SceneBase* scene : viewports.all_scenes)
            scene->markShadowValues();
    }

    // ---- Project Management ----
    void _projectPrepare();
    void _projectStart();
    void _projectStop();
    void _projectPause();
    void _projectDestroy();
    void _projectProcess();
    void _projectDraw();

    std::vector<FingerInfo> pressed_fingers;
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
    static std::shared_ptr<ProjectInfo> createProjectFactoryInfo()
    {
        auto project_info = std::make_shared<ProjectInfo>(T::info());
        project_info->creator = []() -> ProjectBase* { return new T(); };
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
            scene->temporary_environment = config;

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
        scene->temporary_environment = config_ptr;
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
        scene->temporary_environment = config;
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

    [[nodiscard]] int fboWidth() { return canvas->fboWidth(); }
    [[nodiscard]] int fboHeight() { return canvas->fboHeight(); }

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

template<VarBufferConcept VarBufferType>
class Project : public ProjectBase, public DoubleBuffer<VarBufferType>
{
    friend class ProjectBase;

public:

    Project() : ProjectBase(), DoubleBuffer<VarBufferType>()
    {
        has_var_buffer = true;
    }

protected:

    virtual void _projectAttributes() override
    {
        DoubleBuffer<VarBufferType>::shadow_attributes.populateUI();
    }

    void updateLiveBuffers() override
    {
        ProjectBase::updateLiveBuffers(); // call on Scenes
        DoubleBuffer<VarBufferType>::updateLiveBuffer();
    }
    void updateShadowBuffers() override
    {
        ProjectBase::updateShadowBuffers(); // call on Scenes
        DoubleBuffer<VarBufferType>::updateShadowBuffer();
    }
    void markLiveValues() override
    {
        ProjectBase::markLiveValues(); // call on Scenes
        DoubleBuffer<VarBufferType>::markLiveValues();
    }
    void markShadowValues() override
    { 
        ProjectBase::markShadowValues(); // call on Scenes
        DoubleBuffer<VarBufferType>::markShadowValues();
    }
    bool changedShadow() override
    { 
        if (ProjectBase::changedShadow()) return true; // call on Scenes
        return DoubleBuffer<VarBufferType>::changedShadow();
    }
};

typedef ProjectBase BasicProject;
typedef SceneBase BasicScene;

BL_END_NS

#pragma once

#include <vector>
#include <memory>
#include <string>
#include <sstream>
#include <functional>
#include <type_traits>
#include <random>

#include "platform.h"
#include "helpers.h"
#include "types.h"
#include "json.h"

#include "camera.h"
#include "nano_canvas.h"
#include "imgui_custom.h"
#include "project_worker.h"

using std::max;
using std::min;

// Provide macros for easy BasicProject registration
template<typename... Ts>
std::vector<std::string> VectorizeArgs(Ts&&... args) { return { std::forward<Ts>(args)... }; }

#define SIM_BEG(ns) namespace ns{
#define SIM_DECLARE(ns, ...) namespace ns { AutoRegisterProject<ns##_Project> register_##ns(VectorizeArgs(__VA_ARGS__));
#define SIM_END(ns) } using ns::ns##_Project;



/// =================================================================================
/// ======= varcpy overloads (helper for safely copying data between threads) =======
/// =================================================================================

// --- varcpy (non-primative requiring ::copyFrom method) ---
template<typename T>
concept ImpliedThreadSafeCopyable = requires(T t, const T & rhs) {
    { t.copyFrom(rhs) } -> std::same_as<void>;
};

template<ImpliedThreadSafeCopyable T>
void varcpy(T& dst, const T& src) {
    dst.copyFrom(src);
}

// --- varcpy (primative copyable) ---
template<typename T>
concept PrimitiveCopyable = std::is_trivially_copyable_v<T> && std::is_assignable_v<T&, const T&>;

template<PrimitiveCopyable T>
void varcpy(T& dst, const T& src) {
    dst = src;
}

// --- varcpy (trivially copyable array) ---
template<typename T, size_t N>
void varcpy(T(&dst)[N], const T(&src)[N]) {
    static_assert(std::is_trivially_copyable_v<T>,
        "varcpy: array element type must be trivially copyable");
    std::memcpy(dst, src, sizeof(T) * N);
}

// --- varcpy (trivially copyable base part of a possibly non-trivially copyable derived class) ---
template<typename Base, typename Derived>
void varcpy(Derived& dst, const Derived& src) {
    varcpy(*static_cast<Base*>(&dst), *static_cast<const Base*>(&src));
}

/// ---------------------------------------------------------
/// 
/// Tiny helper macros to insert sentinel markers around data
/// which will be automatically synchronized between the live
/// data buffer and GUI data buffer.
/// 
/// Note: Only one sync-block is permitted per VarBuffer.
/// 
/// ---------------------------------------------------------

#define sync_struct std::byte __sync_beg__; struct
#define sync_end    ; std::byte __sync_end__;

template<class T>
constexpr std::span<std::byte> writable_sync_span(T& obj)
{
    auto* beg = reinterpret_cast<std::byte*>(&obj.__sync_beg__) + 1;
    auto* end = reinterpret_cast<std::byte*>(&obj.__sync_end__);
    return { beg, end };
}

template<class T>
constexpr std::span<const std::byte> readonly_sync_span(const T& obj)
{
    auto* beg = reinterpret_cast<const std::byte*>(&obj.__sync_beg__) + 1;
    auto* end = reinterpret_cast<const std::byte*>(&obj.__sync_end__);
    return { beg, end };
}

template<typename T>
concept VarBufferConcept = requires(T t, const T & rhs) {
    { t.populate() } -> std::same_as<void>;
    { t.copyFrom(rhs) } -> std::same_as<void>;
};

template<typename T>
concept HasSyncMembers = requires(T & t) {
    { t.__sync_beg__ } -> std::convertible_to<std::byte>;
    { t.__sync_end__ } -> std::convertible_to<std::byte>;
};

struct VarBuffer
{
    VarBuffer() = default;
    VarBuffer(const VarBuffer&) = delete;
};

class Layout;
class Viewport;
class ProjectBase;
class ProjectManager;
struct ImDebugLog;

class Event
{
    friend class ProjectBase;

    SDL_Event _event;
    Viewport* _focused_ctx = nullptr;
    Viewport* _hovered_ctx = nullptr;
    Viewport* _owner_ctx = nullptr;

protected:

    void setFocusedViewport(Viewport* ctx) {
        _focused_ctx = ctx;
    }
    void setHoveredViewport(Viewport* ctx) {
        _hovered_ctx = ctx;
    }
    void setOwnerViewport(Viewport* ctx) {
        _owner_ctx = ctx;
    }

public:

    Event(const SDL_Event& e) : _event(e) {}

    Viewport* focused_ctx() { return _focused_ctx; }
    Viewport* hovered_ctx() { return _hovered_ctx; }
    Viewport* owner_ctx() { return _owner_ctx; }

    SDL_Event* operator->() { return &_event; }
    const SDL_Event* operator->() const { return &_event; }

    // Touch helpers
    double finger_x();
    double finger_y();

    std::string info();
};

class SceneBase : public VariableChangedTracker
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
    virtual void _updateLiveSceneAttributes() {}
    virtual void _updateShadowSceneAttributes() {}
    //
    
    void _onEvent(Event& e) 
    {
        onEvent(e);
    }

public:

    std::shared_ptr<void> temporary_environment;

    Camera* camera = nullptr;
    MouseInfo* mouse = nullptr;

    struct Config {};

    SceneBase() : gen(std::random_device{}())
    {}
    virtual ~SceneBase() = default;

    void mountTo(Viewport* viewport);
    void mountTo(Layout& viewports);
    void mountToAll(Layout& viewports);

    virtual void sceneStart() {}
    virtual void sceneMounted(Viewport* ctx) {}
    virtual void sceneStop() {}
    virtual void sceneDestroy() {}
    virtual void sceneProcess() {}
    virtual void viewportProcess(Viewport* ctx) {}
    virtual void viewportDraw(Viewport* ctx) const = 0;

    virtual void onEvent(Event& e) {}

    void pollEvents();
    void pollData();

    ///virtual void touchEvent(TouchEvent e) {}
    ///virtual void mouseDown() {}
    ///virtual void mouseUp() {}
    ///virtual void mouseMove() {}
    ///virtual void mouseWheel() {}

    virtual std::string name() const { return "Scene"; }
    int sceneIndex() const { return scene_index; }

    double frame_dt(int average_samples = 1) const;
    double scene_dt(int average_samples = 1) const;
    double project_dt(int average_samples = 1) const;

    double fps(int average_samples = 1) const { return 1000.0 / frame_dt(average_samples); }

    ///FRect combinedViewportsRect()
    ///{
    ///    FRect ret{};
    ///    for (Viewport* ctx : viewports)
    ///    {
    ///        ctx->viewportRect();
    ///    }
    ///}

    double random(double min = 0, double max = 1) const
    {
        std::uniform_real_distribution<> dist(min, max);
        return dist(gen);
    }

    DVec2 Offset(double stage_offX, double stage_offY) const
    {
        return camera->toWorldOffset({ stage_offX, stage_offY });
    }

    void logMessage(const char* fmt, ...);
    void logClear();
};

template<VarBufferConcept VarBufferType>
class Scene : public SceneBase, public VarBufferType
{
    friend class ProjectBase;

    VarBufferType shadow_attributes;

public:

    Scene() : SceneBase(), shadow_attributes()
    {
        has_var_buffer = true;
        ///shadow_attributes.live_attributes = this;
    }


protected:

    virtual void _sceneAttributes() override 
    {
        shadow_attributes.populate();
    }

    void _updateLiveSceneAttributes() override
    {
        if constexpr (HasSyncMembers<VarBufferType>)
        {
            auto dst = writable_sync_span(*static_cast<VarBufferType*>(this));
            auto src = readonly_sync_span(shadow_attributes);
            std::memcpy(dst.data(), src.data(), dst.size());
        }

        // Give sim a chance to manually copy non-trivially copyable data
        VarBufferType* this_attributes = dynamic_cast<VarBufferType*>(this);
        this_attributes->copyFrom(shadow_attributes);
    }

    void _updateShadowSceneAttributes() override
    {
        if constexpr (HasSyncMembers<VarBufferType>)
        {
            auto dst = writable_sync_span(shadow_attributes);
            auto src = readonly_sync_span(*static_cast<VarBufferType*>(this));
            std::memcpy(dst.data(), src.data(), dst.size());
        }

        // Give sim a chance to manually copy non-trivially copyable data
        VarBufferType* this_attributes = dynamic_cast<VarBufferType*>(this);
        shadow_attributes.copyFrom(*this);
    }

    //void _discoverChildBuffers(VarBuffer* live_data, VarBuffer* shadow_data) override
    //{
    //    live_data->child_ptrs[this->getProjectInfo()->sim_uid] = this;
    //    shadow_data->child_ptrs[this->getProjectInfo()->sim_uid] = &shadow_attributes;
    //}
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

public:
    
    double width = 0;
    double height = 0;

    Viewport(
        Layout* layout,
        Canvas *canvas,
        int viewport_index,
        int grid_x,
        int grid_y
    );

    ~Viewport();

    void draw();

    int viewportIndex() { return viewport_index; }
    int viewportGridX() { return viewport_grid_x; }
    int viewportGridY() { return viewport_grid_y; }
    DRect viewportRect() { return DRect(x, y, x + width, y + height);}
    DQuad worldQuad() { return camera.toWorldQuad(0, 0, width, height); }

    template<typename T, typename... Args>
    T* construct(Args&&... args)
    {
        DebugPrint("Scene constructed. Mounting to Viewport: %d", viewport_index);
        scene = new T(std::forward<Args>(args)...);
        scene->registerMount(this);
        return dynamic_cast<T*>(scene);
    }

    template<typename T>
    T* mountScene(T* _sim)
    {
        DebugPrint("Mounting existing scene to Viewport: %d", viewport_index);
        scene = _sim;
        scene->registerMount(this);
        return _sim;
    }

    // Viewport-specific draw helpers (i.e. size of viewport needed)
    void printTouchInfo();
    
    void drawWorldAxis(
        double axis_opacity = 0.3,
        double grid_opacity = 0.04,
        double text_opacity = 0.4
    );

    std::stringstream& print() {
        return print_stream;
    }
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

    void setSize(int targ_viewports_x, int targ_viewports_y)
    {
        this->targ_viewports_x = targ_viewports_x;
        this->targ_viewports_y = targ_viewports_y;
    }

    template<typename T, typename... Args>
    Layout* constructAll(Args&&... args)
    {
        for (Viewport* viewport : viewports)
            viewport->construct<T>(std::forward<Args>(args)...);
        return this;
    }

    Viewport* operator[](size_t i) { expandCheck(i + 1); return viewports[i]; }
    Layout&   operator<<(SceneBase* scene) { scene->mountTo(*this); return *this; }

    iterator begin() { return viewports.begin(); }
    iterator end() { return viewports.end(); }

    const_iterator begin() const { return viewports.begin(); }
    const_iterator end() const { return viewports.end(); }

    int count() const {
        return static_cast<int>(viewports.size());
    }
};

struct SimSceneList : public std::vector<SceneBase*>
{
    void mountTo(Layout& viewports) {
        for (size_t i = 0; i < size(); i++)
            at(i)->mountTo(viewports[i]);
    }
};

struct ProjectInfoNode
{
    std::shared_ptr<ProjectInfo> project_info = nullptr;
    std::vector<ProjectInfoNode> children;
    std::string name;
    bool open = true;

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
    int dt_frameProcess = 0;

    std::chrono::steady_clock::time_point last_frame_time 
        = std::chrono::steady_clock::now();

    Viewport* focused_ctx = nullptr;
    Viewport* hovered_ctx = nullptr;

protected:

    //friend class ProjectManagerCls;
    friend class CProjectWorker;
    friend class Layout;
    friend class SceneBase;

    Canvas* canvas = nullptr;
    ImDebugLog* project_log = nullptr;

    // ----------- States -----------
    bool started = false;
    bool paused = false;
    bool done_single_process = false;

    void configure(int sim_uid, Canvas* canvas, ImDebugLog* project_log);

    DVec2 surfaceSize(); // Dimensions of canvas (or FBO if recording)
    void updateViewportRects();

    // -------- Attributes --------
    bool has_var_buffer = false;
    void _populateAllAttributes();
    virtual void _projectAttributes() {}
    virtual void _updateLiveProjectAttributes() {}
    virtual void _updateShadowProjectAttributes() {}
    //virtual void _discoverChildBuffers(VarBuffer* live_data, VarBuffer* shadow_data) {}

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
    static std::vector<std::shared_ptr<ProjectInfo>>& projectInfoList()
    {
        static std::vector<std::shared_ptr<ProjectInfo>> info_list;
        return info_list;
    }

    static ProjectInfoNode& projectTreeRootInfo()
    {
        static ProjectInfoNode root("root");
        return root;
    }

    static std::shared_ptr<ProjectInfo> findProjectInfo(int sim_uid)
    {
        for (auto& info : projectInfoList())
        {
            if (info->sim_uid == sim_uid)
                return info;
        }
        return nullptr;
    }

    static std::shared_ptr<ProjectInfo> findProjectInfo(const char *name)
    {
        for (auto& info : projectInfoList())
        {
            if (info->path.back() == name)
                return info;
        }
        return nullptr;
    }

    static void addProjectInfo(const std::vector<std::string>& tree_path, const ProjectCreatorFunc& func)
    {
        static int factory_sim_index = 0;

        std::vector<std::shared_ptr<ProjectInfo>>& project_list = projectInfoList();

        auto project_info = std::make_shared<ProjectInfo>(ProjectInfo(
            tree_path,
            func,
            factory_sim_index++,
            ProjectInfo::INACTIVE
        ));

        project_list.push_back(project_info);

        ProjectInfoNode* current = &projectTreeRootInfo();

        // Traverse tree and insert info in correct nested category
        for (size_t i = 0; i < tree_path.size(); i++)
        {
            const std::string& insert_name = tree_path[i];
            bool is_leaf = (i == (tree_path.size() - 1));

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

        std::sort(project_list.begin(), project_list.end(),
            [](auto a, auto b)
        { 
            return a->path < b->path; 
        });
    }

    std::shared_ptr<ProjectInfo> getProjectInfo()
    {
        return findProjectInfo(sim_uid);
    }

    void setProjectInfoState(ProjectInfo::State state);

    /// todo: Find a way to make protected
    void updateAllLiveAttributes()
    {
        _updateLiveProjectAttributes();
        for (SceneBase* scene : viewports.all_scenes)
            scene->_updateLiveSceneAttributes();
    }
    void updateAllShadowAttributes()
    {
        _updateShadowProjectAttributes();
        for (SceneBase* scene : viewports.all_scenes)
            scene->_updateShadowSceneAttributes();
    }

    // Shared Scene creators

    template<typename SceneType>
    SceneType* create()
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
        //scene->setGLFunctions(canvas->getGLFunctions());

        return scene;
    }

    template<typename SceneType> SceneType* create(typename SceneType::Config config)
    {
        auto config_ptr = std::make_shared<typename SceneType::Config>(config);

        SceneType* scene = new SceneType(*config_ptr);
        scene->temporary_environment = config_ptr;
        scene->scene_index = scene_counter++;
        scene->project = this;
        //scene->setGLFunctions(canvas->getGLFunctions());

        return scene;
    }

    template<typename SceneType>
    SceneType* create(std::shared_ptr<typename SceneType::Config> config)
    {
        if (!config)
            throw "Launch Config wasn't created";

        SceneType* scene = new SceneType(*config);
        scene->temporary_environment = config;
        scene->scene_index = scene_counter++;
        scene->project = this;
        //scene->setGLFunctions(canvas->getGLFunctions());

        return scene;
    }

    template<typename SceneType>
    std::shared_ptr<SimSceneList> create(int count)
    {
        auto ret = std::make_shared<SimSceneList>();
        for (int i = 0; i < count; i++)
        {
            SceneType* scene = create<SceneType>();
            scene->project = this;
            //scene->setGLFunctions(canvas->getGLFunctions());

            ret->push_back(scene);
        }
        return ret;
    }

    template<typename SceneType>
    std::shared_ptr<SimSceneList> create(int count, typename SceneType::Config config)
    {
        auto ret = std::make_shared<SimSceneList>();
        for (int i = 0; i < count; i++)
        {
            SceneType* scene = create<SceneType>(config);
            scene->project = this;
            //scene->setGLFunctions(canvas->getGLFunctions());

            ret->push_back(scene);
        }
        return ret;
    }

    Layout& newLayout();
    Layout& newLayout(int _viewports_x, int _viewports_y);
    Layout& currentLayout()
    {
        return viewports;
    }

    int fboWidth() { return canvas->fboWidth(); }
    int fboHeight() { return canvas->fboHeight(); }

    virtual void projectAttributes() {}
    virtual void projectPrepare() = 0;
    virtual void projectStart() {}
    virtual void projectStop() {}
    virtual void projectDestroy() {}

    virtual void onEvent(Event& e) {}

    void pollData();
    void pollEvents();

    void logMessage(const char* fmt, ...);
    void logClear();
};

template<VarBufferConcept VarBufferType>
class Project : public ProjectBase, public VarBufferType
{
    friend class ProjectBase;

    VarBufferType shadow_attributes;

public:

    Project() : ProjectBase(), shadow_attributes()
    {
        has_var_buffer = true;
    }

protected:

    virtual void _projectAttributes() override
    {
        shadow_attributes.populate();
    }

    void _updateLiveProjectAttributes() override
    {
        if constexpr (HasSyncMembers<VarBufferType>)
        {
            auto dst = writable_sync_span(*static_cast<VarBufferType*>(this));
            auto src = readonly_sync_span(shadow_attributes);
            std::memcpy(dst.data(), src.data(), dst.size());
        }

        // Give sim a chance to manually copy non-trivially copyable data
        VarBufferType* this_attributes = dynamic_cast<VarBufferType*>(this);
        this_attributes->copyFrom(shadow_attributes);
    }
    void _updateShadowProjectAttributes() override
    {
        if constexpr (HasSyncMembers<VarBufferType>)
        {
            auto dst = writable_sync_span(shadow_attributes);
            auto src = readonly_sync_span(*static_cast<VarBufferType*>(this));
            std::memcpy(dst.data(), src.data(), dst.size());
        }

        // Give sim a chance to manually copy non-trivially copyable data
        VarBufferType* this_attributes = dynamic_cast<VarBufferType*>(this);
        shadow_attributes.copyFrom(*this);
    }
};

typedef ProjectBase BasicProject;
typedef SceneBase BasicScene;

template <typename T>
struct AutoRegisterProject
{
    AutoRegisterProject(const std::vector<std::string>& tree_path)
    {
        ProjectBase::addProjectInfo(tree_path, []() -> ProjectBase* {
            return (ProjectBase*)(new T());
        });
    }
};

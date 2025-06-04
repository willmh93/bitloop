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
#include "event.h"
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


template<typename T>
concept VarBufferConcept = requires(T t, const T & rhs)
{
    { t.populate() } -> std::same_as<void>;
};

struct BaseVariable
{
    std::string name;

    BaseVariable(std::string_view name) : name(name) {}

    virtual BaseVariable* clone() { return nullptr; }
    virtual std::string toString() { return "<na>"; }
    virtual void setValue(const BaseVariable* rhs) = 0;
    virtual bool equals(const BaseVariable* rhs) const = 0;
};

template<typename T>
struct Variable final : public BaseVariable
{
    using element_type =
        std::conditional_t<std::is_array_v<T>,
        std::remove_extent_t<T>,
        T>;

    using storage_ptr = element_type*;

    storage_ptr ptr = nullptr;
    bool owns_data = false;

    Variable(std::string_view name, storage_ptr v, bool auto_destruct)
        : BaseVariable(name), ptr(v), owns_data(auto_destruct)
    {}

    ~Variable()
    {
        if (owns_data && ptr)
        {
            delete ptr;
            ptr = nullptr;
        }
    }

    void setValue(const BaseVariable* rhs) override
    {
        const auto rhs_var = dynamic_cast<const Variable<T>*>(rhs);
        if (!rhs_var)
            return; // bad cast

        if constexpr (std::is_array_v<T>)
        {
            constexpr std::size_t N = std::extent_v<T>;
            std::copy(rhs_var->ptr, rhs_var->ptr + N, ptr);
        }
        else
        {
            *ptr = *rhs_var->ptr;
        }
    }

    bool equals(const BaseVariable* rhs) const override
    {
        const auto rhs_var = dynamic_cast<const Variable<T>*>(rhs);
        if (!rhs_var)
            return false; // bad cast

        if constexpr (std::is_array_v<T>)
        {
            constexpr std::size_t N = std::extent_v<T>;
            return std::equal(ptr, ptr + N, rhs_var->ptr);
        }
        else
        {
            return *ptr == *rhs_var->ptr;
        }
    }

    Variable* clone() override
    {
        if constexpr (std::is_array_v<T>)
        {
            constexpr std::size_t N = std::extent_v<T>;
            auto* new_arr = new element_type[N];
            std::copy(ptr, ptr + N, new_arr);
            return new Variable(name, new_arr, true);
        }
        else
        {
            return new Variable(name, new element_type(*ptr), true);
        }
    }

    std::string toString()
    {
        std::stringstream ss;
        ss << *ptr;
        return ss.str();
    }
};

struct VariableEntry
{
    int id;
    std::string name;

    // Actual live/shadow buffers
    BaseVariable* shadow_ptr    = nullptr;
    BaseVariable* live_ptr      = nullptr;

    // "Before" buffers to compare against and detect changes
    BaseVariable* shadow_marked = nullptr;
    BaseVariable* live_marked   = nullptr;

    // Track whether shadow value has been edited by the GUI and is awaiting
    // processing on the live side. 'version' increments on every GUI edit so
    // external code can detect fresh changes.
    bool dirty = false;
    unsigned int version = 0;
    unsigned int live_version = 0; // last processed version on live side

    // Once a live worker change is detected, don't immediately push to shadow buffer.
    // The live buffer reacted to now-expired data, so don't override the latest shadow changes
    // 
    // shadowChanged is broken because you no longer "mark" shadow before _sceneAttributes
    // 
    //
    // -- backup (but complex) solution --
    // or... do allow immediately pushing to shadow buffer, but if ImGui makes changes,
    // package those changes up in a "pending shadow update" which overrides it.
    // If the world panning/rotation/zoom makes changes, they are live changes which
    // get overriden by the pending package
    //
    // This means your current "shadow_ptr" is in-fact being used more like "pending changes"
    // for the ACTUAL shadow buffer.
    // Make sceneAttributes act on a copy of the shadow_ptr (the pending package), and if
    // changes are detected on it, push them to shadow_ptr
    // 
    // -- another backup solution --
    // Make each variable hashable for easy comparison. Then you can take light "snapshots"
    // to see if a variable changed
    // > The challenge: Be able to hash any type T value
    // > At the moment, you expect every T/T[] to be comparable with ==
    // > Which means you'd need to abandon that and find some way to force T to be hashable
    // > You'd still be needing each T to provide operator=

    void updateLive()      { /* if (id==1) DebugPrint("live:          cam_x = %s", shadow_ptr->toString().c_str()); */ live_ptr->setValue(shadow_ptr); }
    void updateShadow()    { /* if (id==1) DebugPrint("shadow:        cam_x = %s", live_ptr->toString().c_str());   */ shadow_ptr->setValue(live_ptr); }
    void markLiveValue()   { /* if (id==1) DebugPrint("marked_live:   cam_x = %s", live_ptr->toString().c_str());   */ live_marked->setValue(live_ptr); }
    void markShadowValue() { /* if (id==1) DebugPrint("marked_shadow: cam_x = %s", shadow_ptr->toString().c_str()); */ shadow_marked->setValue(shadow_ptr); }
    bool liveChanged()   const  { return !live_ptr->equals(live_marked); }
    bool shadowChanged() const  { return !shadow_ptr->equals(shadow_marked); }

    void print()
    {
        DebugPrint("Shadow: %s  Live: %s", 
            shadow_ptr->toString().c_str(),
            live_ptr->toString().c_str()
        );
    }
};

struct VariableMap : public std::vector<VariableEntry>
{
    void markLiveValues()   { for (size_t i=0; i<size(); i++) at(i).markLiveValue(); }
    void markShadowValues() { for (size_t i=0; i<size(); i++) at(i).markShadowValue(); }
    void markDirtyIfShadowChanged()
    {
        for (auto& entry : *this)
        {
            if (entry.shadowChanged())
            {
                entry.dirty = true;
                ++entry.version;
            }
        }
    }
    bool changedShadow() {
        for (size_t i = 0; i < size(); i++)  {
            if (at(i).shadowChanged())
                return true;
        }
        return false;
    }
};

struct VarTracker
{
    std::vector<BaseVariable*> buffer_var_ptrs;

    template<typename T>
    void _sync(const char*name, T& v)
    {
        if constexpr (std::is_array_v<T>)
            buffer_var_ptrs.push_back(new Variable<T>(name, v, false));   // v  -> element*
        else
            buffer_var_ptrs.push_back(new Variable<T>(name, &v, false));  // &v -> T*
    }

    #define sync(v) _sync(#v, v)

    /// Ownership of ptrs claimed by Scene
};

struct VarBuffer : public VarTracker
{
    VarBuffer() = default;
    VarBuffer(const VarBuffer&) = delete;

    virtual void setup() {};
    virtual void populate() {}
};

template<typename T>
concept DerivedFromVarBuffer = std::derived_from<T, VarBuffer>;

template<DerivedFromVarBuffer VarBufferType>
class DoubleBuffer : public VarBufferType
{
public:

    VarBufferType shadow_attributes;
    VariableMap var_map;


    DoubleBuffer() : shadow_attributes()
    {
        // Let each buffer list their synced members with sync()
        shadow_attributes.setup();
        VarBufferType::setup();

        // Join up the lists into a single 'var_map'
        for (size_t i = 0; i < VarBufferType::buffer_var_ptrs.size(); i++)
        {
            VariableEntry entry;
            entry.shadow_ptr = shadow_attributes.buffer_var_ptrs[i];
            entry.live_ptr = VarBufferType::buffer_var_ptrs[i];
            entry.shadow_marked = entry.shadow_ptr->clone();
            entry.live_marked = entry.live_ptr->clone();

            entry.id = (int)i;
            entry.name = entry.live_ptr->name;
            //entry.print();

            var_map.push_back(entry);
        }

        // Taken ownership of pointers, clear lists
        shadow_attributes.buffer_var_ptrs.clear();
        VarBufferType::buffer_var_ptrs.clear();
    }

    ~DoubleBuffer()
    {
        // todo: Clean up owned data pointers
    }

    void updateLiveBuffer()
    {
        DebugPrint("updateLiveBuffers()");
        for (VariableEntry& entry : var_map) {
            entry.updateLive();
            entry.live_version = entry.version;
            if (entry.dirty)
                entry.dirty = false;
        }
    }
    void updateShadowBuffer()
    {
        DebugPrint("updateShadowBuffers()");
        for (VariableEntry& entry : var_map) {
            if (entry.live_version < entry.version)
                continue; // GUI edit not yet processed on live side
            if (entry.dirty)
                continue;
            if (entry.liveChanged())
                entry.updateShadow();
        }
    }

    void markDirtyVariables() { var_map.markDirtyIfShadowChanged(); }

    void markLiveValues() { var_map.markLiveValues(); }
    void markShadowValues() { var_map.markShadowValues(); }

    bool changedShadow() { return var_map.changedShadow(); }

    std::string pad(const std::string& str, int width) const
    {
        return str + std::string(width > str.size() ? width - str.size() : 0, ' ');
    }

    std::string toString() const
    {
        static const int name_len = 28;
        static const int val_len = 10;

        std::stringstream txt;
        for (const auto& entry : var_map)
            txt << pad(std::to_string(entry.id) + ". ", 4) << pad(entry.name, name_len)
            << "SHADOW_MARKED: " << pad(entry.shadow_marked->toString(), val_len) << "   SHADOW_VAL: " << pad(entry.shadow_ptr->toString(), val_len)
            << "    LIVE_MARKED:   " << pad(entry.live_marked->toString(), val_len) << "   LIVE_VAL: " << pad(entry.live_ptr->toString(), val_len)
            << "\n";

        return txt.str();
    }
};

class Layout;
class Viewport;
class ProjectBase;
class ProjectManager;
struct ImDebugLog;


class SceneBase : public VariableChangedTracker//, public VarTracker
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
    virtual bool changedShadow() { return false; }
    virtual void markLiveValues() {}
    virtual void markShadowValues() {}
    virtual void markDirtyVariables() {}

    //
    
    void _onEvent(Event e) 
    {
        onEvent(e);

        switch (e.type())
        {
        case SDL_FINGERDOWN:       onPointerDown(PointerEvent(e));  break;
        case SDL_MOUSEBUTTONDOWN:  onPointerDown(PointerEvent(e));  break;
        case SDL_FINGERUP:         onPointerUp(PointerEvent(e));    break;
        case SDL_MOUSEBUTTONUP:    onPointerUp(PointerEvent(e));    break;
        case SDL_FINGERMOTION:     onPointerMove(PointerEvent(e));  break;
        case SDL_MOUSEMOTION:      onPointerMove(PointerEvent(e));  break;
        case SDL_MOUSEWHEEL:       onWheel(PointerEvent(e));        break;

        case SDL_KEYDOWN:          onKeyDown(KeyEvent(e));          break;
        case SDL_KEYUP:            onKeyUp(KeyEvent(e));            break;
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
    virtual void sceneMounted(Viewport* ctx) {}
    virtual void sceneStop() {}
    virtual void sceneDestroy() {}
    virtual void sceneProcess() {}
    virtual void viewportProcess(Viewport* ctx) {}
    virtual void viewportDraw(Viewport* ctx) const = 0;

    virtual void onEvent(Event e) {}

    void pollEvents();
    void pullDataFromShadow();

    virtual void onPointerEvent(PointerEvent e) {}
    virtual void onPointerDown(PointerEvent e) {}
    virtual void onPointerUp(PointerEvent e) {}
    virtual void onPointerMove(PointerEvent e) {} 
    virtual void onWheel(PointerEvent e) {}
    virtual void onKeyDown(KeyEvent e) {}
    virtual void onKeyUp(KeyEvent e) {}

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
        return camera->toWorldOffset({ stage_offX, stage_offY });
    }

    void logMessage(std::string_view, ...);
    void logClear();
};

template<DerivedFromVarBuffer VarBufferType>
class Scene : public SceneBase, public DoubleBuffer<VarBufferType>
{
    friend class ProjectBase;

public:

    Scene() : SceneBase(), DoubleBuffer<VarBufferType>()
    {
        has_var_buffer = true;
    }

protected:

    virtual void _sceneAttributes() override 
    {
        DoubleBuffer<VarBufferType>::shadow_attributes.populate();
    }

    void updateLiveBuffers() override   { DoubleBuffer<VarBufferType>::updateLiveBuffer(); }
    void updateShadowBuffers() override { DoubleBuffer<VarBufferType>::updateShadowBuffer(); }
    void markLiveValues() override      { DoubleBuffer<VarBufferType>::markLiveValues();   }
    void markShadowValues() override    { DoubleBuffer<VarBufferType>::markShadowValues(); }
    void markDirtyVariables() override  { DoubleBuffer<VarBufferType>::markDirtyVariables(); }
    bool changedShadow() override       { return DoubleBuffer<VarBufferType>::changedShadow(); }
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

    [[nodiscard]] int viewportIndex() { return viewport_index; }
    [[nodiscard]] int viewportGridX() { return viewport_grid_x; }
    [[nodiscard]] int viewportGridY() { return viewport_grid_y; }
    [[nodiscard]] DRect viewportRect() { return DRect(x, y, x + width, y + height); }
    [[nodiscard]] DQuad worldQuad() { return camera.toWorldQuad(0, 0, width, height); }

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

    void setSize(int targ_viewports_x, int targ_viewports_y)
    {
        this->targ_viewports_x = targ_viewports_x;
        this->targ_viewports_y = targ_viewports_y;
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
    int dt_frameProcess 

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

    virtual void markDirtyVariables()
    {
        for (SceneBase* scene : viewports.all_scenes)
            scene->markDirtyVariables();
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
    static [[nodiscard]] std::vector<std::shared_ptr<ProjectInfo>>& projectInfoList()
    {
        static std::vector<std::shared_ptr<ProjectInfo>> info_list;
        return info_list;
    }

    static [[nodiscard]] ProjectInfoNode& projectTreeRootInfo()
    {
        static ProjectInfoNode root("root");
        return root;
    }

    static [[nodiscard]] std::shared_ptr<ProjectInfo> findProjectInfo(int sim_uid)
    {
        for (auto& info : projectInfoList())
        {
            if (info->sim_uid == sim_uid)
                return info;
        }
        return nullptr;
    }

    static [[nodiscard]] std::shared_ptr<ProjectInfo> findProjectInfo(const char *name)
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

    virtual void projectAttributes() {}
    virtual void projectPrepare(Layout& layout) = 0;
    virtual void projectStart() {}
    virtual void projectStop() {}
    virtual void projectDestroy() {}

    virtual void onEvent(Event& e) {}

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
        DoubleBuffer<VarBufferType>::shadow_attributes.populate();
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
    void markDirtyVariables() override
    {
        ProjectBase::markDirtyVariables();
        DoubleBuffer<VarBufferType>::markDirtyVariables();
    }
    bool changedShadow() override
    {
        if (ProjectBase::changedShadow()) return true; // call on Scenes
        return DoubleBuffer<VarBufferType>::changedShadow();
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

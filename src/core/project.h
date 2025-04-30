#pragma once

#include <vector>
#include <memory>
#include <string>
#include <sstream>
#include <functional>
#include <random>

#include "helpers.h"
#include "types.h"
#include "camera.h"
#include "nano_canvas.h"
#include "imgui_custom.h"

using std::max;
using std::min;

// Provide macros for easy Project registration
template<typename... Ts>
std::vector<std::string> VectorizeArgs(Ts&&... args) { return { std::forward<Ts>(args)... }; }

#define SIM_BEG(ns) namespace ns{
#define SIM_DECLARE(ns, ...) namespace ns { AutoRegisterProject<ns##_Project> register_##ns(VectorizeArgs(__VA_ARGS__));
#define SIM_END(ns) } using ns::ns##_Project;

class Project;
class Layout;
class Viewport;
class Project;
class ProjectManager;
struct ImDebugLog;

class Scene : public VariableChangedTracker
{
    std::random_device rd;
    std::mt19937 gen;

protected:

    friend class Viewport;
    friend class Project;

    Project* project = nullptr;

    int scene_index = -1;
    std::vector<Viewport*> mounted_to_viewports;

    // Mounting to/from viewport
    void registerMount(Viewport* viewport);
    void registerUnmount(Viewport* viewport);

public:

    std::shared_ptr<void> temporary_environment;

    Camera* camera = nullptr;
    MouseInfo* mouse = nullptr;

    struct Config {};

    Scene() : gen(rd())
    {
    }
    virtual ~Scene() = default;

    void mountTo(Viewport* viewport);
    void mountTo(Layout& viewports);
    void mountToAll(Layout& viewports);

    virtual void sceneAttributes() {}
    virtual void sceneStart() {}
    virtual void sceneMounted(Viewport* ctx) {}
    virtual void sceneStop() {}
    virtual void sceneDestroy() {}
    virtual void sceneProcess() {}

    virtual void viewportProcess(Viewport* ctx) {}
    virtual void viewportDraw(Viewport* ctx) = 0;

    virtual void mouseDown() {}
    virtual void mouseUp() {}
    virtual void mouseMove() {}
    virtual void mouseWheel() {}

    //virtual void keyPressed(QKeyEvent* e) {};
    //virtual void keyReleased(QKeyEvent* e) {};

    virtual std::string name() { return "Scene"; }
    int sceneIndex() { return scene_index; }

    double random(double min = 0, double max = 1)
    {
        std::uniform_real_distribution<> dist(min, max);
        return dist(gen);
    }

    Vec2 Offset(double stage_offX, double stage_offY)
    {
        return camera->toWorldOffset({ stage_offX, stage_offY });
    }

    void logMessage(const char* fmt, ...);
    void logClear();
};

class Viewport : public Painter
{
protected:

    friend class Project;
    friend class Scene;
    friend class Layout;
    friend class Camera;

    int viewport_index;
    int viewport_grid_x;
    int viewport_grid_y;

    Layout* layout = nullptr;
    Scene* scene = nullptr;

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

    template<typename T, typename... Args>
    T* construct(Args&&... args)
    {
        ///qDebug() << "Scene constructed. Mounting to Viewport: " << viewport_index;
        scene = new T(std::forward<Args>(args)...);
        scene->registerMount(this);
        return dynamic_cast<T*>(scene);
    }

    template<typename T>
    T* mountScene(T* _sim)
    {
        ///qDebug() << "Mounting existing scene to Viewport: " << viewport_index;

        scene = _sim;
        scene->registerMount(this);
        return _sim;
    }

    // Viewport-specific draw helpers (i.e. size of viewport needed)
    void drawWorldAxis(
        double axis_opacity = 0.3,
        double grid_opacity = 0.04,
        double text_opacity = 0.4
    );
};

class Layout
{
    std::vector<Viewport*> viewports;

protected:

    friend class Project;
    friend class Scene;

    // If 0, viewports expand in that direction. If both 0, expand whole grid.
    int targ_viewports_x = 0;
    int targ_viewports_y = 0;
    int cols = 0;
    int rows = 0;

    std::vector<Scene*> all_scenes;
    Project* project = nullptr;

public:

    using iterator = typename std::vector<Viewport*>::iterator;
    using const_iterator = typename std::vector<Viewport*>::const_iterator;

    ~Layout()
    {
        // Only invoked when you switch project
        clear();
    }

    const std::vector<Scene*>& scenes()
    {
        return all_scenes;
    }

    void clear()
    {
        // layout freed each time you call setLayout
        for (Viewport* p : viewports)
            delete p;

        viewports.clear();
    }

    void setSize(int targ_viewports_x, int targ_viewports_y)
    {
        this->targ_viewports_x = targ_viewports_x;
        this->targ_viewports_y = targ_viewports_y;
    }

    void add(
        int _viewport_index,
        int _grid_x,
        int _grid_y);

    template<typename T, typename... Args>
    Layout* constructAll(Args&&... args)
    {
        for (Viewport* viewport : viewports)
            viewport->construct<T>(std::forward<Args>(args)...);
        return this;
    }

    Viewport* operator[](size_t i)
    {
        expandCheck(i + 1);
        return viewports[i];
    }

    Layout& operator <<(Scene* scene)
    {
        scene->mountTo(*this);
        return *this;
    }

    void resize(size_t viewport_count);
    void expandCheck(size_t count);

    iterator begin() { return viewports.begin(); }
    iterator end() { return viewports.end(); }

    const_iterator begin() const { return viewports.begin(); }
    const_iterator end() const { return viewports.end(); }

    int count() const {
        return static_cast<int>(viewports.size());
    }
};

struct SimSceneList : public std::vector<Scene*>
{
    void mountTo(Layout& viewports)
    {
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

class Project
{
    Layout viewports;
    int scene_counter = 0;
    int sim_uid = -1;

protected:

    friend class ProjectManager;
    friend class Layout;
    friend class Scene;

    Canvas* canvas = nullptr;
    ImDebugLog* debug_log = nullptr;

    bool started = false;
    bool paused = false;

    bool done_single_process = false;
    bool done_single_populate_attributes = false;

    void configure(int sim_uid, Canvas* canvas, ImDebugLog* debug_log);

    void updateViewportRects();
    Vec2 surfaceSize(); // Dimensions of canvas (or FBO if recording)

    void _populateAttributes();
    void _projectPrepare();
    void _projectStart();
    void _projectStop();
    void _projectPause();
    void _projectDestroy();
    void _projectProcess();
    void _projectDraw();

public:

    MouseInfo mouse;

    virtual ~Project() = default;
    
    // Project Factory methods
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

    static void addProjectInfo(const std::vector<std::string>& tree_path, const CreatorFunc& func)
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

    int canvasWidth() { return canvas->width(); }
    int canvasHeight() { return canvas->height(); }

    virtual void projectAttributes() {}
    virtual void projectPrepare() = 0;
    virtual void projectStart() {}
    virtual void projectStop() {}
    virtual void projectDestroy() {}

    void logMessage(const char* fmt, ...);
    void logClear();
};

template <typename T>
struct AutoRegisterProject
{
    AutoRegisterProject(const std::vector<std::string>& tree_path)
    {
        DebugMessage("AutoRegisterProject() called");
        Project::addProjectInfo(tree_path, []() -> Project* {
            return (Project*)(new T());
        });
    }
};

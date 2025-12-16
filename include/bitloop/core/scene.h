#pragma once

#include <random>

#include <bitloop/platform/platform.h>
#include <bitloop/core/capture_manager.h>
#include <bitloop/core/snapshot_presets.h>
#include <bitloop/util/math_util.h>
#include <bitloop/util/change_tracker.h>
#include <bitloop/nanovgx/nano_canvas.h>

#include "types.h"
#include "event.h"
#include "var_buffer.h"
#include "camera.h"
#include "input.h"

BL_BEGIN_NS

class Viewport;
class ProjectBase;
class Layout;



class SceneBase : public ChangeTracker
{
    mutable std::mt19937 gen;

    int dt_sceneProcess = 0;

    mutable std::vector<Math::MovingAverage::MA<double>> dt_scene_ma_list;
    mutable std::vector<Math::MovingAverage::MA<double>> dt_project_ma_list;
    //mutable std::vector<Math::MovingAverage::MA> dt_project_draw_ma_list;

    mutable size_t dt_call_index = 0;
    mutable size_t dt_process_call_index = 0;
    //size_t dt_draw_call_index = 0;

    mutable bool initiating_snapshot = false;
    mutable bool initiating_recording = false;


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
    virtual void _populateOverlay() {}

    // In case Scene uses double buffer
    virtual void updateLiveBuffers() {}
    virtual void updateShadowBuffers() {}
    virtual bool changedLive() { return false; }
    virtual bool changedShadow() { return false; }
    virtual void markLiveValues() {}
    virtual void markShadowValues() {}
    virtual void updateUnchangedShadowVars() {}
    virtual void invokeScheduledCalls() {}

    //
    

    void _onEvent(Event e) 
    {
        onEvent(e);

        if (ownsEvent(e))
        {
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
    }

public:

    // config kept alive for at least as long as this Scene holds the shared_ptr to the launch config
    // see:  ProjectBase::create(typename SceneType::Config config)
    std::shared_ptr<void> active_config;

    MouseInfo* mouse = nullptr;

    //Input input;

    struct Config {};
    struct UI {};

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

    bool ownsEvent(const Event& e);
    virtual void onEvent(Event) {}

    virtual void onPointerEvent(PointerEvent) {}
    virtual void onPointerDown(PointerEvent) {}
    virtual void onPointerUp(PointerEvent) {}
    virtual void onPointerMove(PointerEvent) {}
    virtual void onWheel(PointerEvent) {}
    virtual void onKeyDown(KeyEvent) {}
    virtual void onKeyUp(KeyEvent) {}

    virtual std::string name() const { return "Scene"; }
    [[nodiscard]] int sceneIndex() const { return scene_index; }

    [[nodiscard]] double frame_dt(int average_samples = 1) const;
    [[nodiscard]] double scene_dt(int average_samples = 1) const;
    [[nodiscard]] double project_dt(int average_samples = 1) const;

    [[nodiscard]] double fps(int average_samples = 1) const { return 1000.0 / frame_dt(average_samples); }
    [[nodiscard]] double fpsFactor() const;

    [[nodiscard]] bool isSnapshotting() const;
    [[nodiscard]] bool isRecording() const;
    [[nodiscard]] bool isCapturing() const;
    [[nodiscard]] bool capturedLastFrame();

    void beginSnapshot(const SnapshotPresetList& presets, const char* relative_filepath = nullptr);
    void beginRecording();
    void endRecording();

    void permitCaptureFrame(bool b);
   
    [[nodiscard]] double random(double min = 0, double max = 1) const
    {
        std::uniform_real_distribution<> dist(min, max);
        return dist(gen);
    }

    void logMessage(std::string_view, ...);
    void logClear();
};

template<typename SceneType>
class Scene : public SceneBase, public VarBuffer<SceneType>
{
    friend class ProjectBase;

public:

    using Interface = VarBufferInterface<SceneType>;
    Interface* ui = nullptr;

    Scene() : SceneBase()
    {
        has_var_buffer = true;
        ui = new SceneType::UI(static_cast<const SceneType*>(this));
        ui->init();
    }

    ~Scene()
    {
        delete ui;
    }

protected:

    void _sceneAttributes() override final
    {
        ui->sidebar();
    }

    void _populateOverlay() override final
    {
        ui->overlay();
    }

private:

    void updateLiveBuffers() override final          { VarBuffer<SceneType>::updateLive(); }
    void updateShadowBuffers() override final        { VarBuffer<SceneType>::updateShadow(); }
    void markLiveValues() override final             { VarBuffer<SceneType>::markLiveValue(); }
    void markShadowValues() override final           { VarBuffer<SceneType>::markShadowValue(); }
    bool changedLive() override final                { return VarBuffer<SceneType>::liveChanged(); }
    bool changedShadow() override final              { return VarBuffer<SceneType>::shadowChanged(); }
    void updateUnchangedShadowVars() override final  { VarBuffer<SceneType>::updateUnchangedShadowVars(); }
    void invokeScheduledCalls() override final       { VarBuffer<SceneType>::invokeScheduledCalls(); }
};

// todo: Make dedicated "SingleThreadedScene" class which uses a single thread for both UI/process
//       BasicScene currently doesn't support any UI
typedef SceneBase BasicScene;

BL_END_NS

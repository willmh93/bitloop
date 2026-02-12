#pragma once

#include <random>
#include <chrono>

#include <bitloop/platform/platform.h>
#include <bitloop/core/interface_model.h>
#include <bitloop/core/capture_manager.h>
#include <bitloop/core/snapshot_presets.h>
#include <bitloop/util/math_util.h>
#include <bitloop/util/change_tracker.h>
#include <bitloop/util/timer.h>
#include <bitloop/nanovgx/nano_canvas.h>

#include "types.h"
#include "event.h"

#include "camera.h"
#include "input.h"

BL_BEGIN_NS


class Viewport;
class ProjectBase;
class Layout;

class SceneBase : public ChangeTracker
{
    friend class ProjectBase;

    mutable std::mt19937 gen;

    double dt_sceneProcess = 0;
    SimpleTimer running_time;
    
    // mutable so we return true the moment we begin capturing,
    // even if CaptureManager hasn't registered it yet.
    mutable bool initiating_snapshot = false;
    mutable bool initiating_recording = false;

    int capture_request_id = 1; // 0 reserved for "no callback"
    std::vector<std::pair<int, SnapshotBatchCallbacks>> snapshot_callbacks;

    bool needs_redraw = false;

    virtual void _initGUI() {}
    virtual void _destroyGUI() {}

//protected:

    friend class Viewport;
    friend class ProjectBase;

    ProjectBase* project = nullptr;

    int scene_index = -1;
    std::vector<Viewport*> mounted_to_viewports;

    bool started = false;
    bool destroyed = false;

    void _sceneStart()
    {
        started = true;
        running_time.begin();
        sceneStart();
    }

    void _sceneDestroy()
    {
        if (!destroyed && started)
        {
            sceneDestroy();
            destroyed = true;
        }
    }

    // Mounting to/from viewport
    void registerMount(Viewport* viewport);
    void registerUnmount(Viewport* viewport);

    //
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

    void _onEncodeFrame(EncodeFrame& data, int request_id, const CapturePreset& preset);

    //

    void _onEvent(Event e) 
    {
        onEvent(e);

        if (e.ownedBy(this))
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

    // todo: Give each viewport it's own mouse, don't spill position between viewports
    MouseInfo* mouse = nullptr;

    //Input input;

    struct Config {};
    struct UI {};

    SceneBase() : gen(std::random_device{}()) {}
    virtual ~SceneBase() = default;

    virtual InterfaceModel* getInterfaceModel() = 0;

    void mountTo(Viewport* viewport);
    void mountTo(Layout& viewports);
    void mountToAll(Layout& viewports);

    void requestRedraw(bool b) { needs_redraw = b; }
    void requestImmediateUpdate();

    virtual void sceneStart() {}
    virtual void sceneMounted(Viewport*) {}
    virtual void sceneStop() {}
    virtual void sceneDestroy() {}
    virtual void sceneProcess() {}
    virtual void viewportProcess(Viewport*, double) {}
    virtual void viewportDraw(Viewport*) const = 0;
    virtual void onEndFrame() {}

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
    [[nodiscard]] const std::vector<Viewport*>& mountedToViewports() const {
        return mounted_to_viewports;
    }

    [[nodiscard]] double running_dt() const { return running_time.elapsed(); }
    [[nodiscard]] double scene_dt() const { return dt_sceneProcess; } // scene only (not including frame delay)
    [[nodiscard]] double project_dt() const; // project + all scenes (not including frame delay)
    [[nodiscard]] double frame_dt() const; // whole frame time elapsed (including sleep)

    [[nodiscard]] double fps() const { return 1000.0 / frame_dt(); }
    [[nodiscard]] double fpsFactor() const; // rate multier:  1x is real FPS = target FPS

    [[nodiscard]] bool isSnapshotting() const;
    [[nodiscard]] bool isRecording() const;
    [[nodiscard]] bool isCapturing() const;
    [[nodiscard]] bool capturedLastFrame() const;
    [[nodiscard]] int  capturedFrameCount() const;

    void permitCaptureFrame(bool b);

    virtual void onBeginSnapshot() {}
    virtual void onEncodeFrame(EncodeFrame& data [[maybe_unused]], const CapturePreset& preset [[maybe_unused]]) {}

    void beginSnapshotList(
        const SnapshotPresetList& presets,
        std::string_view relative_filepath = {},
        SnapshotCompleteCallback on_snapshot_complete = nullptr,
        SnapshotBatchCompleteCallback on_batch_complete = nullptr);

    void beginSnapshot(
        const CapturePreset& preset,
        std::string_view relative_filepath = {},
        SnapshotCompleteCallback on_snapshot_complete = nullptr)
    {
        beginSnapshotList(SnapshotPresetList(preset), relative_filepath, on_snapshot_complete, nullptr);
    }

    void beginRecording();
    void endRecording();

    [[nodiscard]] double random(double min = 0, double max = 1) const
    {
        std::uniform_real_distribution<> dist(min, max);
        return dist(gen);
    }

    void logMessage(std::string_view, ...);
    void logClear();
};

class BasicScene : public SceneBase, public DirectInterfaceModel
{
    void _sceneAttributes() override final
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


template<typename SceneType>
class Scene : public SceneBase, public VarBuffer<SceneType>
{
    friend class ProjectBase;

public:

    using BufferedInterfaceModel = DoubleBufferedInterfaceModel<SceneType>;
    BufferedInterfaceModel* ui = nullptr;
    
    Scene() : SceneBase()
    {
    }

    ~Scene()
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
        return static_cast<const SceneType::UI*>(ui);
    }

    void _initGUI() override final
    {
        if (!ui) {
            // safer to initialize UI on same thread as UI populate
            ui = new SceneType::UI(static_cast<const SceneType*>(this));
            ui->init();
        }
    }

    void _destroyGUI() override final
    {
        if (ui)
        {
            ui->destroy();
            delete ui;
            ui = nullptr;
        }
    }

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


BL_END_NS

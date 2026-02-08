#include <bitloop/core/scene.h>
#include <bitloop/core/project.h>
#include <bitloop/core/viewport.h>
#include <bitloop/core/layout.h>
#include <bitloop/core/main_window.h>

BL_BEGIN_NS

void SceneBase::registerMount(Viewport* viewport)
{
    mounted_to_viewports.push_back(viewport);
    // Mounted to Viewport: viewport->viewportIndex()

    std::vector<SceneBase*>& all_scenes = viewport->layout->all_scenes;

    // Only add scene to list if it's not already in the layout (mounted to another viewport)
    if (std::find(all_scenes.begin(), all_scenes.end(), this) == all_scenes.end())
        viewport->layout->all_scenes.push_back(this);
}

void SceneBase::registerUnmount(Viewport* viewport)
{
    std::vector<SceneBase*>& all_scenes = viewport->layout->all_scenes;

    // Only remove scene from list if the layout includes it (which it should)
    auto scene_it = std::find(all_scenes.begin(), all_scenes.end(), this);
    if (scene_it != all_scenes.end())
    {
        // Erasing project scene from viewport
        all_scenes.erase(scene_it);
    }

    // Only unmount from viewport if the layout includes the viewport (which it should)
    auto viewport_it = std::find(mounted_to_viewports.begin(), mounted_to_viewports.end(), viewport);
    if (viewport_it != mounted_to_viewports.end())
    {
        // Unmounting scene from viewport
        mounted_to_viewports.erase(viewport_it);
    }
}

void SceneBase::mountTo(Viewport* viewport)
{
    viewport->mountScene(this);
}

void SceneBase::mountTo(Layout& viewports)
{
    Viewport* ctx = viewports[viewports.count()];
    ctx->mountScene(this);

    if (!this->project->ctx_focused)
        this->project->ctx_focused = ctx;
}

void SceneBase::mountToAll(Layout& viewports)
{
    for (Viewport* viewport : viewports)
        viewport->mountScene(this);
}

void SceneBase::requestImmediateUpdate()
{
    project->requestImmediateUpdate();
}

double SceneBase::frame_dt() const
{
    return project->frame_dt;
}

double SceneBase::project_dt() const
{
    return project->project_dt;
}

double SceneBase::fpsFactor() const
{
    if (!main_window()->isFixedFrameTimeDelta())
    {
        double actual_fps = fps();
        double target_fps = main_window()->getFPS();

        return (target_fps / actual_fps);
    }
    else
    {
        return 1.0;
    }
}

bool SceneBase::isSnapshotting() const
{
    bool ret = main_window()->isSnapshotting();
    if (ret) initiating_snapshot = false;
    return ret || initiating_snapshot;
}

bool SceneBase::isRecording() const
{
    bool ret = main_window()->getCaptureManager()->isRecording();
    if (ret) initiating_recording = false;
    return ret || initiating_recording;
}

bool SceneBase::isCapturing() const
{
    return isSnapshotting() || isRecording();
}

void SceneBase::_onEncodeFrame(EncodeFrame& data, int request_id, const CapturePreset& preset)
{
    onEncodeFrame(data, preset);

    for (size_t i = 0; i < snapshot_callbacks.size(); i++)
    {
        const auto& [callback_id, batch_callbacks] = snapshot_callbacks[i];
        const auto& [on_snapshot_complete, on_batch_complete] = batch_callbacks;

        if (callback_id == request_id)
        {
            if (on_snapshot_complete)
                on_snapshot_complete(data, preset);

            if (on_batch_complete && i == snapshot_callbacks.size() - 1)
                on_batch_complete();

            snapshot_callbacks.erase(snapshot_callbacks.begin() + i);
            return;
        }
    }
}

void SceneBase::beginSnapshotList(
    const SnapshotPresetList& presets,
    std::string_view rel_path_fmt,
    SnapshotCompleteCallback on_snapshot_complete,
    SnapshotBatchCompleteCallback on_batch_complete)
{
    snapshot_callbacks.push_back(std::make_pair(capture_request_id, SnapshotBatchCallbacks{ on_snapshot_complete, on_batch_complete }));

    main_window()->queueBeginSnapshot(presets, rel_path_fmt, capture_request_id);

    capture_request_id++;
    initiating_snapshot = true;
}

void SceneBase::beginRecording()
{
    main_window()->queueBeginRecording();
    initiating_recording = true;
}

void SceneBase::endRecording()
{
    main_window()->queueEndRecording();
    initiating_recording = false;
}

void SceneBase::permitCaptureFrame(bool b)
{
    main_window()->captureFrame(b);
}

bool SceneBase::capturedLastFrame() const
{
    return main_window()->capturedLastFrame();
}

int SceneBase::capturedFrameCount() const
{
    return main_window()->capturedFrameCount();
}


void SceneBase::logClear()
{
    project->project_log->clear();
}

void SceneBase::logMessage(std::string_view fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    project->project_log->vlog(fmt.data(), ap);
    va_end(ap);
}

BL_END_NS

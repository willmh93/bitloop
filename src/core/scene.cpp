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
    viewports[viewports.count()]->mountScene(this);
}

void SceneBase::mountToAll(Layout& viewports)
{
    for (Viewport* viewport : viewports)
        viewport->mountScene(this);
}

bool SceneBase::ownsEvent(const Event& e)
{
    return e.ctx_owner() && e.ctx_owner()->scene == this;
}

double SceneBase::frame_dt(int average_samples) const
{
    if (dt_call_index >= dt_scene_ma_list.size())
        dt_scene_ma_list.push_back(Math::MovingAverage::MA<double>(average_samples));

    auto& ma = dt_scene_ma_list[dt_call_index++];
    return ma.push(project->dt_frameProcess);
}

double SceneBase::scene_dt(int average_samples) const
{
    if (dt_call_index >= dt_scene_ma_list.size())
        dt_scene_ma_list.push_back(Math::MovingAverage::MA<double>(average_samples));

    auto& ma = dt_scene_ma_list[dt_call_index++];
    return ma.push(dt_sceneProcess);
}

double SceneBase::project_dt(int average_samples) const
{
    if (dt_process_call_index >= dt_project_ma_list.size())
        dt_project_ma_list.push_back(Math::MovingAverage::MA<double>(average_samples));

    auto& ma = dt_project_ma_list[dt_process_call_index++];
    return ma.push(project->dt_projectProcess);
}

double SceneBase::fpsFactor() const
{
    if (!main_window()->isFixedFrameTimeDelta())
    {
        double actual_fps = 1000.0 / frame_dt(1);
        double target_fps = main_window()->getFPS();
        return (target_fps / actual_fps);
    }
    else
    {
        return 1.0;
    }
}

bool SceneBase::isRecording() const
{
    return main_window()->getRecordManager()->isRecording();
}

void SceneBase::queueBeginRecording()
{
    main_window()->queueBeginRecording();
}

void SceneBase::queueEndRecording()
{
    main_window()->queueEndRecording();
}

void SceneBase::captureFrame(bool b)
{
    main_window()->captureFrame(b);
}

bool SceneBase::capturedLastFrame()
{
    return main_window()->capturedLastFrame();
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

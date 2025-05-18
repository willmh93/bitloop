#pragma once

#include "imgui_custom.h"
#include "nano_canvas.h"
#include "project.h"

class ProjectManagerInternal
{
    ProjectBase* active_project = nullptr;
    Canvas* canvas = nullptr;
    ImDebugLog* project_log = nullptr;

    VarBuffer live_data;
    VarBuffer shadow_data;

    static ProjectManagerInternal* singleton;

public:

    static ProjectManagerInternal* get()
    {
        if (singleton == nullptr)
            singleton = new ProjectManagerInternal();
        return singleton;
    }

    void updateLiveAttributes()
    {
        if (active_project)
            active_project->updateAllLiveAttributes();
    }

    void updateShadowAttributes()
    {
        if (active_project)
            active_project->updateAllShadowAttributes();
    }

    void process()
    {
        if (active_project)
            active_project->_projectProcess();
    }

    void draw()
    {
        if (active_project)
            active_project->_projectDraw();
    }

    void setSharedCanvas(Canvas* shared_canvas)
    {
        canvas = shared_canvas;
    }

    void setSharedDebugLog(ImDebugLog* shared_log)
    {
        project_log = shared_log;
    }

    void setActiveProject(int sim_uid)
    {
        project_log->clear();

        if (active_project)
        {
            active_project->_projectDestroy();
            delete active_project;
            active_project = nullptr;
        }

        DebugPrint("-------- Switching Project --------");
        active_project = ProjectBase::findProjectInfo(sim_uid)->creator();
        active_project->configure(sim_uid, canvas, project_log);
        active_project->_projectPrepare();
    }

    ProjectBase* getActiveProject()
    {
        return active_project;
    }

    void destroyProject()
    {
        if (active_project)
        {
            active_project->_projectDestroy();
            delete active_project;
            active_project = nullptr;
        }
    }

    void startProject()
    {
        if (!active_project)
            return;

        active_project->_projectDestroy();
        active_project->_projectStart();
    }

    void stopProject()
    {
        if (!active_project)
            return;

        active_project->_projectDestroy();
        active_project->_projectStop();
    }

    void pauseProject()
    {
        if (active_project)
            active_project->_projectPause();
    }

    void populateAttributes(bool show_ui)
    {
        if (active_project)
            active_project->_populateAllAttributes(show_ui);
    }

    void _onEvent(SDL_Event& e)
    {
        if (active_project)
            active_project->_onEvent(e);
    }
};

static ProjectManagerInternal* ProjectManager()
{
    return ProjectManagerInternal::get();
}
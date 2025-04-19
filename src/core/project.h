#pragma once

#include <vector>
#include <memory>
#include <string>
#include <functional>

#include "nanovg_canvas.h"

class Project;
using CreatorFunc = std::function<Project*()>;

struct ProjectInfo
{
    enum State
    {
        INACTIVE,
        ACTIVE,
        RECORDING
    };

    std::vector<std::string> path;
    CreatorFunc creator;
    int sim_uid;
    State state;

    ProjectInfo(
        std::vector<std::string> path,
        CreatorFunc creator = nullptr,
        int sim_uid = -100,
        State state = State::INACTIVE
    )
        : path(path), creator(creator), sim_uid(sim_uid), state(state)
    {}
};

class Project
{
    static std::vector<std::shared_ptr<ProjectInfo>>& projectInfoList()
    {
        static std::vector<std::shared_ptr<ProjectInfo>> info_list;
        return info_list;
    }

    static void addProjectInfo(const std::vector<std::string>& tree_path, const CreatorFunc& func)
    {
        static int factory_sim_index = 0;
        projectInfoList().push_back(std::make_shared<ProjectInfo>(ProjectInfo(
            tree_path,
            func,
            factory_sim_index++,
            ProjectInfo::INACTIVE
        )));
    }

public:

    virtual void process() {}
    virtual void draw(Canvas* canvas) {}
};

template <typename T>
struct AutoRegisterProject
{
    AutoRegisterProject(const std::vector<std::string>& tree_path)
    {
        Project::addProjectInfo(tree_path, []() -> Project* {
            return (Project*)(new T());
        });
    }
};

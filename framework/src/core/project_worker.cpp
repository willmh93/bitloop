#include <core/project_worker.h>
#include <core/main_window.h>
#include <core/project.h>

BL_BEGIN_NS

ProjectWorker* ProjectWorker::singleton = nullptr;

void ProjectWorker::startWorker()
{
    worker_thread = std::thread(&ProjectWorker::worker_loop, singleton);
    shared_sync.project_thread_started = true;
}

void ProjectWorker::end()
{
    if (shared_sync.project_thread_started)
    {
        // Kill worker thread
        worker_thread.join();

        // Clean up
        _destroyActiveProject();
    }
}


void ProjectWorker::_destroyActiveProject()
{
    if (active_project)
    {
        active_project->_projectDestroy();
        delete active_project;
        active_project = nullptr;
    }
}


void ProjectWorker::handleProjectCommands(ProjectCommandEvent& e)
{
    switch (e.type)
    {
    case ProjectCommandType::PROJECT_SET:
    {
        project_log.clear();

        _destroyActiveProject();

        active_project = ProjectBase::findProjectInfo(e.project_uid)->creator();
        active_project->configure(e.project_uid, MainWindow::instance()->getCanvas(), &project_log);
        active_project->_projectPrepare();
    }
    break;

    case ProjectCommandType::PROJECT_START:
        if (active_project)
        {
            active_project->_projectDestroy();
            active_project->_projectStart();

            active_project->updateShadowBuffers();
        }
        break;

    case ProjectCommandType::PROJECT_STOP:
        if (active_project)
        {
            active_project->_projectDestroy();
            active_project->_projectStop();
        }
        break;

    case ProjectCommandType::PROJECT_PAUSE:
        if (active_project)
        {
            active_project->_projectPause();
        }
        break;
    }
}

void ProjectWorker::worker_loop()
{
    bool shadow_changed = false;

    while (!shared_sync.quitting.load())
    {
        /// ======== Safe place to control project changes ========
        if (!project_command_queue.empty())
        {
            // We don't call populateAttributes() if holding shadow_buffer_mutex,
            // meaning we won't loop over scenes here while processing project commands
            std::unique_lock<std::mutex> shadow_lock(shared_sync.shadow_buffer_mutex);

            for (auto& e : project_command_queue) handleProjectCommands(e);
            project_command_queue.clear();
        }

        // Wait for GUI to consume the freshly-rendered previous frame
        shared_sync.wait_until_gui_consumes_frame();


        /// ======== Do heavy work (while GUI thread redraws cached frame) ========
        if (active_project) 
        {
            /// ======== Update live values ========
            {
                //blPrint() << "----------------------------";
                //blPrint() << "----- NEW WORKER FRAME -----";

                // ======== Event polling ========
                {
                    active_project->markLiveValues();

                    // If shadow data changed (due to ImGui inputs), discard pending SDL events since ImGui
                    // already processed those events internally and they shouldn't be treated as canvas input
                    bool discard_event_batch = shadow_changed;
                    pollEvents(discard_event_batch);

                    // If handled SDL event changed live buffer, immediately push those changes to shadow buffer
                    if (active_project->changedLive())
                        pushDataToShadow();
                }
                
                // ======== Update Live Buffer ========
                pullDataFromShadow();

                // ======== Mark buffer states prior to process ========
                {
                    std::unique_lock<std::mutex> shadow_lock(shared_sync.shadow_buffer_mutex);
                    active_project->markLiveValues();
                    active_project->markShadowValues();
                }

                // ======== Process simulation (potentially heavy work) ========
                active_project->_projectProcess();

                
                // ======== Update shadow buffer with *changed* live variables ========
                {
                    shadow_changed = active_project->changedShadow();
                    if (!shadow_changed)
                        pushDataToShadow();
                }

                //blPrint() << "----- END WORKER FRAME -----";
                //blPrint() << "----------------------------";
                //blPrint() << "";
            }
        }

        /// ======== Flag ready to draw ========
        shared_sync.flag_ready_to_draw();

        /// ======== Wake GUI ========
        shared_sync.cv.notify_one();

        // Start draw timer
        auto draw_t0 = std::chrono::steady_clock::now();

        // Wait for GUI to draw the freshly prepared data
        shared_sync.wait_until_gui_drawn();


        // Wait 16ms (minus time taken to draw the frame)
        //using namespace std::chrono;
        //auto draw_duration = steady_clock::now() - draw_t0;
        //int draw_ms = static_cast<int>(duration_cast<milliseconds>(draw_duration).count());
    }
}

void ProjectWorker::pushDataToShadow()
{
    // While we update the shadow buffer, we should NOT 
    // permit ui updates (force a small stall)
    std::unique_lock<std::mutex> shadow_lock(shared_sync.shadow_buffer_mutex);
    active_project->updateShadowBuffers();
}

void ProjectWorker::pullDataFromShadow()
{
    //if (shared_sync.editing_ui.load())
    {
        /// Make sure we can't alter the shadow buffer while we copy them to live buffer
        std::lock_guard<std::mutex> live_buffer_lock(shared_sync.live_buffer_mutex);
        shared_sync.updating_live_buffer.store(true);

        active_project->updateLiveBuffers();

        shared_sync.updating_live_buffer.store(false, std::memory_order_release);

        // Inform GUI thread that 'updating_live_buffer' changed
        shared_sync.cv_updating_live_buffer.notify_one();
    }
}

void ProjectWorker::queueEvent(const SDL_Event& event)
{
    std::lock_guard<std::mutex> lock(event_queue_mutex);
    input_event_queue.push_back(event);
}

void ProjectWorker::pollEvents(bool discardBatch)
{
    // Grab event queue data (and clear via swap with empty queue)
    std::vector<SDL_Event> local;
    {
        std::lock_guard<std::mutex> lock(event_queue_mutex);
        local.swap(input_event_queue);
    }

    if (!discardBatch)
    {
        for (SDL_Event& e : local)
            _onEvent(e);
    }
}

void ProjectWorker::populateAttributes()
{
    if (active_project)
        active_project->_populateAllAttributes();
}

void ProjectWorker::_onEvent(SDL_Event& e)
{
    if (active_project)
        active_project->_onEvent(e);
}

void ProjectWorker::draw()
{
    if (active_project)
        active_project->_projectDraw();
}

BL_END_NS

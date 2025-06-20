#include "project_worker.h"
#include "main_window.h"
#include "project.h"

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
    case PROJECT_SET:
    {
        project_log.clear();

        _destroyActiveProject();

        active_project = ProjectBase::findProjectInfo(e.project_uid)->creator();
        active_project->configure(e.project_uid, MainWindow::instance()->getCanvas(), &project_log);
        active_project->_projectPrepare();
    }
    break;

    case PROJECT_START:
        if (active_project)
        {
            active_project->_projectDestroy();
            active_project->_projectStart();

            active_project->updateShadowBuffers();
        }
        break;

    case PROJECT_STOP: 
        if (active_project)
        {
            active_project->_projectDestroy();
            active_project->_projectStop();
        }
        break;

    case PROJECT_PAUSE: 
        if (active_project)
        {
            active_project->_projectPause();
        }
        break;
    }
}

void ProjectWorker::worker_loop()
{
    DebugPrint("worker_loop started");

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
        {
            // Preventing switching project entirely during a full worker frame
            std::lock_guard<std::mutex> lock(shared_sync.working_mutex);

            /// ======== Update live values ========
            // in case any inputs were changed since last .process()
            shared_sync.processing_frame.store(true);
            {
                //DebugPrint("----------------------------");
                //DebugPrint("----- NEW WORKER FRAME -----");

                // If projectProcess() didn't change cam_x/cam_y, how do you allow panning while rotating?
                // the goal is to allow overriding what projectProcess did, but not if ImGui changed something.
                // issue: shadow now contains live changes, so it will think a UI change occured even if it didn't?
                {
                    if (active_project)
                        active_project->markLiveValues();

                    ///DebugPrint("Polling Events");
                    bool discard_events = shadow_changed;// active_project&& active_project->changedShadow();
                    pollEvents(discard_events);

                    // todo: if (live_changed) from before pollEvents?
                    //if (!shadow_changed)

                    if (active_project && active_project->changedLive())
                        pushDataToShadow(); // only pushes updated cam_rot to shadow
                }

                /*if (shared_sync.gui_populated_during_process.load())
                {
                    // GUI shadow change must have occured during last worker frame,
                    // meaning shadow still contains an unsynced change
                    DebugPrint("Found unsynced change");
                }*/

                /// apply shadow data *changes* TO live buffer
                //if (!shared_sync.gui_populated_during_process.load())
                //{
                    //DebugPrint("pullDataFromShadow()");
                    pullDataFromShadow();

                    

                //}

                //if (active_project && active_project->changedShadow())
                //{
                //    DebugPrint("DETECTED SHADOW CHANGE");
                //}

                
                if (active_project)
                {
                    active_project->markLiveValues();
                    //if (!shared_sync.gui_populated_during_process.load()) // I don't think this is doing anything
                    ///    active_project->markShadowValues();

                    //if (!shared_sync.gui_populated_during_process.load())


                    std::unique_lock<std::mutex> shadow_lock(shared_sync.shadow_buffer_mutex);
                    active_project->markShadowValues();
                }



                // If worker frame completes without GUI overriding value, updating shadow
                // at the end of *this* frame is allowed
                //shared_sync.gui_populated_during_process.store(false);



                //DebugPrint("projectProcess()");
                if (active_project)
                    active_project->_projectProcess(); // changes cam_rot

                

                ///if (active_project)
                ///    active_project->commitVariableChangeTracker();

                // Then don't update shadow OR marked_shadow if the live variables



                 // apply live data *changes* TO shadow buffer
                //if (!shared_sync.gui_populated_during_process.load())
                {
                    //DebugPrint("pushDataToShadow()");
                    shadow_changed = active_project && active_project->changedShadow();
                    if (!shadow_changed)
                        pushDataToShadow(); // pushes ONLY the updated cam_rot to shadow
                }
                ///else
                ///{
                ///    DebugPrint("pushDataToShadow() SKIPPED (GUI change means we still need that shadow update)");
                ///}
  


                //DebugPrint("----- END WORKER FRAME -----");
                //DebugPrint("----------------------------");
                //DebugPrint("");
            }
            shared_sync.processing_frame.store(false);
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

void ProjectWorker::queueEvent(const SDL_Event& event)
{
    std::lock_guard<std::mutex> lock(event_queue_mutex);
    input_event_queue.push_back(event);
}

void ProjectWorker::pushDataToShadow()
{
    //if (!shared_sync.editing_ui.load())
    {
        // While we update the shadow buffer, we should NOT 
        // permit ui updates (force a small stall)
        std::unique_lock<std::mutex> shadow_lock(shared_sync.shadow_buffer_mutex);

        //if (shadow_lock.owns_lock())
        {
            if (active_project)
            {
                active_project->updateShadowBuffers();
                //DebugPrint("pushDataToShadow::markShadowValues");
                //active_project->markShadowValues();
            }
        }
    }
}

void ProjectWorker::pullDataFromShadow()
{
    //if (shared_sync.editing_ui.load())
    {
        /// Make sure we can't alter the shadow buffer while we copy them to live buffer
        std::lock_guard<std::mutex> live_buffer_lock(shared_sync.live_buffer_mutex);
        shared_sync.updating_live_buffer.store(true);

        if (active_project)
            active_project->updateLiveBuffers();

        shared_sync.updating_live_buffer.store(false, std::memory_order_release);

        // Inform GUI thread that 'updating_live_buffer' changed
        shared_sync.cv_updating_live_buffer.notify_one();
    }
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
        {
            //if (e.type == SDL_MOUSEBUTTONUP)
            //    continue;

            _onEvent(e);
        }
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
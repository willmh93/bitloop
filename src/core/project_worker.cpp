#include <bitloop/core/project_worker.h>
#include <bitloop/core/main_window.h>
#include <bitloop/core/project.h>
#include <bitloop/core/capture_manager.h>

BL_BEGIN_NS

ProjectWorker* ProjectWorker::singleton = nullptr;

void ProjectWorker::startWorker()
{
    worker_thread = std::thread(&ProjectWorker::worker_loop, singleton);
    shared_sync.project_thread_started = true;
}

void ProjectWorker::end()
{
    BL_TAKE_OWNERSHIP("live");

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
    BL_TAKE_OWNERSHIP("live");

    if (current_project)
    {
        current_project->_projectDestroy();
        delete current_project;
        current_project = nullptr;
    }
}


void ProjectWorker::handleProjectCommands(ProjectCommandEvent& e)
{
    BL_TAKE_OWNERSHIP("live");

    switch (e.type)
    {
    case ProjectCommandType::PROJECT_SET:
    {
        project_log.clear();

        _destroyActiveProject();

        current_project = ProjectBase::findProjectInfo(e.project_uid)->creator();
        current_project->configure(e.project_uid, main_window()->getCanvas(), &project_log);
        current_project->_projectPrepare();
    }
    break;

    case ProjectCommandType::PROJECT_PLAY:
        if (current_project)
        {
            if (current_project->isPaused())
            {
                // don't destroy if simply resuming project
                current_project->_projectResume();
            }
            else
            {
                current_project->_projectDestroy();
                current_project->_projectStart();
            }


            current_project->updateShadowBuffers();

            main_window()->queueMainWindowCommand({ MainWindowCommandType::ON_PLAY_PROJECT });
        }
        break;

    case ProjectCommandType::PROJECT_STOP:
        if (current_project)
        {
            current_project->_projectDestroy();
            current_project->_projectStop();

            main_window()->queueMainWindowCommand({ MainWindowCommandType::ON_STOPPED_PROJECT });
        }
        break;

    case ProjectCommandType::PROJECT_PAUSE:
        if (current_project)
        {
            current_project->_projectPause();
            main_window()->queueMainWindowCommand({ MainWindowCommandType::ON_PAUSED_PROJECT });
        }
        break;
    }
}

void ProjectWorker::worker_loop()
{
    //bool shadow_changed = false;

    while (!shared_sync.quitting.load())
    {
        /// ────── Safe place to control project changes ──────
        // We don't call populateAttributes() if holding shadow_buffer_mutex,
        // meaning we won't loop over scenes here while processing project commands
        {
            std::vector<ProjectCommandEvent> commands;
            {
                std::lock_guard<std::mutex> lock(command_mutex);
                commands = std::move(project_command_queue);
            }

            if (!commands.empty())
            {
                std::unique_lock<std::mutex> shadow_lock(shared_sync.shadow_buffer_mutex);
                for (auto& e : commands) handleProjectCommands(e);
            }
        }

        // Wait for GUI to consume the freshly-rendered previous frame
        shared_sync.wait_until_gui_consumes_frame();

        capture_manager->setCaptureEnabled(true);

        // Start draw timer
        auto draw_t0 = std::chrono::steady_clock::now();

        /// ────── Do heavy work (while GUI thread redraws cached frame) ──────
        if (current_project && current_project->started) 
        {
            /// ────── Update live values ──────
            {
                //blPrint() << "──────────────────────────────";
                //blPrint() << "────── NEW WORKER FRAME ──────";

                /// ────── Event polling ──────
                {
                    current_project->markLiveValues();

                    pollEvents();

                    // If handled SDL event changed live buffer, immediately push those changes to shadow buffer
                    if (current_project->changedLive())
                        pushDataToShadow();
                }
                
                /// ────── Update Live Buffer ──────
                pullDataFromShadow();

                /// ────── Mark buffer states prior to process ──────
                {
                    std::unique_lock<std::mutex> shadow_lock(shared_sync.shadow_buffer_mutex);
                    current_project->markLiveValues();
                    current_project->markShadowValues();
                }

                // Capture by default unless sim overrides flag (only has an affect when recording)
                if (!current_project->paused)
                    main_window()->captureFrame(true);

                current_project->invokeScheduledCalls();

                /// ────── Process simulation (potentially heavy work) ──────
                current_project->_projectProcess();

                
                /// ────── Update shadow buffer with *changed* live variables ──────
                {
                    // currently, we push live changes to shadow IF shadow itself wasn't changed by UI
                    ///shadow_changed = current_project->changedShadow();
                    ///if (!shadow_changed)
                    ///  pushDataToShadow();
                    pushDataToUnchangedShadowVars();
                }


                //blPrint() << "────── END WORKER FRAME ──────";
                //blPrint() << "──────────────────────────────";
                //blPrint() << "";
            }
        }

        // If recording, don't wake GUI until capture_manager is ready to encode a new frame,
        // otherwise the main GUI thread would need to block while it waits for the last frame to encode.
        // We intentionally avoid a frame queue so video encoding stays in sync with simulation.
        if (main_window()->capturingNextFrame())
        {
            if ((capture_manager->isRecording() || capture_manager->isSnapshotting()))
                capture_manager->waitUntilReadyForNewFrame();
        }

        /// ────── Flag ready to draw ──────
        shared_sync.flag_ready_to_draw();

        /// ────── Wake GUI ──────
        shared_sync.cv.notify_one();

        /// ────── Wait for GUI to draw the freshly prepared data ──────
        shared_sync.wait_until_gui_consumes_frame();

        // Wait 16ms (minus time taken to draw the frame)
        using namespace std::chrono;
        auto draw_duration = steady_clock::now() - draw_t0;
        Uint64 draw_ns = static_cast<Uint64>(duration_cast<nanoseconds>(draw_duration).count());
        Uint64 target_ns = 1000000000llu / main_window()->getFPS();
        Uint64 delay_ns = std::max(0llu, target_ns - draw_ns);
        if (delay_ns > 0)
            SDL_DelayPrecise(delay_ns);
    }
}

void ProjectWorker::pushDataToShadow()
{
    // While we update the shadow buffer, we should NOT 
    // permit ui updates (force a small stall)
    std::unique_lock<std::mutex> shadow_lock(shared_sync.shadow_buffer_mutex);
    current_project->updateShadowBuffers();
}

void ProjectWorker::pushDataToUnchangedShadowVars()
{
    std::unique_lock<std::mutex> shadow_lock(shared_sync.shadow_buffer_mutex);
    current_project->updateUnchangedShadowVars();
}

void ProjectWorker::pullDataFromShadow()
{
    //if (shared_sync.editing_ui.load())
    {
        /// Make sure we can't alter the shadow buffer while we copy them to live buffer
        std::lock_guard<std::mutex> live_buffer_lock(shared_sync.live_buffer_mutex);
        shared_sync.updating_live_buffer.store(true);

        current_project->updateLiveBuffers();

        shared_sync.updating_live_buffer.store(false, std::memory_order_release);

        // Inform GUI thread that 'updating_live_buffer' changed
        shared_sync.cv_updating_live_buffer.notify_one();
    }
}

void ProjectWorker::queueEvent(const SDL_Event& event)
{
    // Queue events which should only be processed once the current worker frame completes
    {
        std::lock_guard<std::mutex> lock(event_queue_mutex);
        input_event_queue.push_back(event);
    }

    // Immediately process events for the overlay
}

void ProjectWorker::pollEvents()
{
    // Grab event queue data (and clear via swap with empty queue)
    std::vector<SDL_Event> local;
    {
        std::lock_guard<std::mutex> lock(event_queue_mutex);
        local.swap(input_event_queue);
    }

    //if (current_project)
    //    current_project->_clearEventQueue();

    for (SDL_Event& e : local)
        _onEvent(e);
}

bool bl::ProjectWorker::hasActiveProject()
{
    return (current_project && current_project->isActive());
}

void ProjectWorker::populateAttributes()
{
    if (current_project)
        current_project->_populateAllAttributes();
}

void ProjectWorker::populateOverlay()
{
    if (current_project)
        current_project->_populateOverlay();
}

void ProjectWorker::onEncodeFrame(bytebuf& data, int request_id, const SnapshotPreset& preset)
{
    if (current_project)
        current_project->_onEncodeFrame(data, request_id, preset);
}

void ProjectWorker::_onEvent(SDL_Event& e)
{
    BL_TAKE_OWNERSHIP("live");

    if (current_project)
        current_project->_onEvent(e);
}

void ProjectWorker::draw()
{
    BL_TAKE_OWNERSHIP("live");

    if (current_project)
        current_project->_projectDraw();
}

BL_END_NS

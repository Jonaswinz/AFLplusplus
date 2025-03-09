#include "afl_client.h"

afl_client::afl_client(int mode, int vp_instances, std::string vp_executable, std::string vp_launch_args, std::string target_path, int vp_loglevel, std::string vp_logging_path, int fksrv_st_fd, int fksrv_ctl_fd){
    m_mode = mode;

    // 0: Restarting mode, 1: Persistent mode
    if(m_mode == 0){
        if(vp_instances > MAX_VP_INSTANCES){
            LOG_MESSAGE(logger::ERROR, "More than %d instances are not supported yet.", MAX_VP_INSTANCES);
            // TODO differently
            exit(1);
        }
        m_vp_clients_count = vp_instances;

    }else if(m_mode == 1){
        m_vp_clients_count = 1;

    }else{

        LOG_MESSAGE(logger::ERROR, "Mode %d not supported!", mode);
        // TODO differently
        exit(1);
    }
    
    m_vp_executable = vp_executable;
    m_vp_launch_args = vp_launch_args;
    m_target_path = target_path;
    m_vp_loglevel = vp_loglevel;
    m_vp_logging_path = vp_logging_path;
    m_fksrv_st_fd = fksrv_st_fd;
    m_fksrv_ctl_fd = fksrv_ctl_fd;

    // Setting the instance to this one.
    instance = this;
};

void afl_client::instance_restarter(int core_id, vp_client** clients_pointer, int clients_count){

    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(core_id, &cpuset);

    // Setting core affinity of this thread to another CPU.
    pthread_t thread = pthread_self();  // Get current thread ID
    if (pthread_setaffinity_np(thread, sizeof(cpu_set_t), &cpuset) != 0) {
        LOG_MESSAGE(logger::ERROR, "INSTANCE_RESTARTER: Failed to set thread affinity of restarter thread!");
    }

    LOG_MESSAGE(logger::INFO, "INSTANCE_RESTARTER: Restarter thread succesfully started!");

    // Forever check the vp_clients if the process need to be restarted.
    while(true){
        
        for(int i=0; i<clients_count; i++){
            
            // Synchronize the access to the vp_clients to not cause issues, when multiple restarter threads are working.
            std::unique_lock<std::mutex> lock(restarter_mutex);
            if(clients_pointer[i]->vp_process_state == vp_client::DONE){
                
                // Set the state to killing so no other restarter thread will pick this client to also do the restarting.
                clients_pointer[i]->vp_process_state = vp_client::KILLING;

                // Release the lock so the other restarter threads can check the others.
                lock.unlock();

                LOG_MESSAGE(logger::INFO, "INSTANCE_RESTARTER: Restarting instance %d.", i);
                clients_pointer[i]->restart_process();
                LOG_MESSAGE(logger::INFO, "INSTANCE_RESTARTER: Done restarting %d.", i);
            }
        }
    }
}

void afl_client::start(uint64_t mmio_address, std::string start_breakpoint, std::string end_breakpoint, std::string return_code_register,  int shm_cov_id, int shm_input_id) {

    // TODO logging seperation of different processes

    // Start the vp clients and processes after another the frist time.
    for(int i=0; i<m_vp_clients_count; i++){
        // Using mmio_address ass the start and end address of the mmio tracking (because we are only interested in this specific address).
        m_vp_clients[i] = new vp_client(m_vp_executable, m_vp_loglevel, m_vp_logging_path, m_vp_launch_args, m_target_path, mmio_address, mmio_address);
        m_vp_clients[i]->start_process();
        m_vp_clients[i]->waiting_for_ready();
        m_vp_clients[i]->setup();
    }

    // TODO only in not persistent / snapshotting mode!
    // TODO restart with only one instance do in the same thread and not instance restarter !?
    
    // The instance restarter will now take care of the restarting, but only when more that one vp client instance is used.
    if (m_vp_clients_count > 1){
        std::thread([=]() {
            instance_restarter(10, m_vp_clients, m_vp_clients_count);
        }).detach();
    }

    m_shm_cov_id = shm_cov_id;
    m_shm_input_id = shm_input_id;

    // Variable holding the index of the current used vp_client inside the m_vp_clients array.
    m_vp_clients_index = 0;

    // Communicate initial status
    EASY_BLOCK("Communicate initial status");
        int status = FS_OPT_ENABLED | FS_OPT_SHDMEM_FUZZ;
        if (write(m_fksrv_st_fd, &status, sizeof(status)) != sizeof(status)) {
            LOG_MESSAGE(logger::ERROR, "Not running in forkserver mode, just executing the program.");
        }
    EASY_END_BLOCK

    // Read response from AFL
    EASY_BLOCK("Read response from AFL");
        int read_status;
        if (read(m_fksrv_ctl_fd, &read_status, sizeof(read_status)) != sizeof(read_status)) {
            LOG_MESSAGE(logger::ERROR, "AFL parent exited before forkserver was up.");
            shutdown();
            exit(1);
        } else if (read_status != status) {
            LOG_MESSAGE(logger::INFO, "Read response from AFL: %d need %d", read_status, status);
            LOG_MESSAGE(logger::ERROR, "Unexpected response from AFL++ on forkserver setup.");
            shutdown();
            exit(1);
        }
    EASY_END_BLOCK

    do{

        LOG_MESSAGE(logger::INFO, "Using instance %d.", m_vp_clients_index);

        // TODO: dont know for what this is for !?
        int child_killed;
        if (read(m_fksrv_ctl_fd, &child_killed, sizeof(child_killed)) != sizeof(child_killed)) {
            LOG_MESSAGE(logger::ERROR, "AFL parent exited before we could fork.");
            shutdown();
            exit(1);
        }

        LOG_MESSAGE(logger::INFO, "Child Killed: %d", child_killed);

        if (child_killed > 0) {
            
            //TODO: recessary ?
            /*
            int status;
            waitpid(-1, &status, WNOHANG); // Simplified waiting
            status = swapEndian(status);
            write(m_fksrv_st_fd, &status, sizeof(status));
            kill();
            exit(1);
            */
        }

        EASY_BLOCK("Communicate child ID");

            pid_t current_child = m_vp_clients[m_vp_clients_index]->vp_process;

            // Get pid and write it to AFL
            if (write(m_fksrv_st_fd, &current_child, sizeof(current_child)) != sizeof(current_child)) {
                LOG_MESSAGE(logger::ERROR, "Failed to communicate with AFL.");
                shutdown();
                exit(1);
            }
        EASY_END_BLOCK

        m_vp_clients[m_vp_clients_index]->do_run(mmio_address, start_breakpoint, end_breakpoint, return_code_register, shm_input_id, 4);
        m_vp_clients[m_vp_clients_index]->get_return_code();


        LOG_MESSAGE(logger::INFO, "Writing code coverage");
        m_vp_clients[m_vp_clients_index]->write_code_coverage(shm_cov_id, 0);

        LOG_MESSAGE(logger::INFO, "Finished, sending status!");

        EASY_BLOCK("Communicate return value");  
            int status = m_vp_clients[m_vp_clients_index]->get_return_code_value();
            if (write(m_fksrv_st_fd, &status, sizeof(status)) != sizeof(status)) {
                LOG_MESSAGE(logger::ERROR, "Failed to send status to AFL.");
                shutdown();
                exit(1);
            }
        EASY_END_BLOCK


        // Only restart whole VP process if restarting mode is enabled.
        if(m_mode == 0){

            LOG_MESSAGE(logger::INFO, "Moving to next instance!");

            m_vp_clients[m_vp_clients_index]->vp_process_state = vp_client::DONE;
            
            // If more than one vp instance is used, the instance restarter thread will do the restarting, so we just need to select one ready instance.
            if(m_vp_clients_count > 1){
                // Bussy searching for an instance where the VP process state is READY and thus it can be used!
                while(true){
                    m_vp_clients_index = (m_vp_clients_index + 1) % m_vp_clients_count;

                    std::unique_lock<std::mutex> lock(restarter_mutex);
                    if(m_vp_clients[m_vp_clients_index]->vp_process_state == vp_client::READY) break;
                    lock.unlock();
                }
            }else{
                // If only one instance is used, do the restarting here, because there is no instance restarter thread.
                m_vp_clients[m_vp_clients_index]->restart_process();
            }
        }


        //Only profile a fixed number of runs
        #ifdef PROFILER_ENABLED
            _profiler_count ++;
            if(_profiler_count == PROFILING_COUNT){
                LOG_MESSAGE(logger::INFO, "Writing profiling file.");
                profiler::dumpBlocksToFile("test_client.prof");
            }
        #endif
    
    }while(true);

    shutdown();
}

void afl_client::shutdown(){
    //TODO SIGTERM or SIGKILL ?
    for(int i=0; i<m_vp_clients_count; i++){
        m_vp_clients[i]->kill_process();
    }
}

void afl_client::signal_handler(int sig) {
    // Handle SIGTERM or other signals as needed
    LOG_MESSAGE(logger::ERROR, "Terminating on signal %d", sig);

    // killing current vp instance
    if(instance != nullptr){
        instance->shutdown();
    }

    exit(0);
}

afl_client* afl_client::instance = nullptr;
std::mutex afl_client::restarter_mutex;
#ifndef AFL_CLIENT_H
#define AFL_CLIENT_H

#include <sys/sysinfo.h>

#include "defines.h"
#include "logger.h"
#include "vp_client.h"
#include "settings.h"

class afl_client{

    public:

        // The afl_client is a singleton. This reference is used for the signal handler.
        static afl_client* instance;

        // Creates a afl_client instance with its parameters.
        afl_client(int fksrv_st_fd, int fksrv_ctl_fd);

        // Static thread that watches a vp_client array and restarts them if needed.
        static void instance_restarter(int core_id, vp_client** clients_pointer, int clients_count);
        
        // Starts the forkserver client.
        void start(const char* m_target_path, int shm_cov_id, int shm_input_id);

        void shutdown();
        
        static void signal_handler(int sig);

    private:
        
        // Maximum of MAX_VP_INSTANCES instances.
        vp_client* m_vp_clients[MAX_VP_INSTANCES];
        int m_vp_clients_index = 0;

        int _profiler_count = 0;
        int m_fksrv_st_fd = -1;
        int m_fksrv_ctl_fd = -1;

        int m_shm_cov_id = -1;
        int m_shm_input_id = -1;

        static std::mutex restarter_mutex;
};

#endif
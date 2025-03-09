#ifndef AFL_CLIENT_H
#define AFL_CLIENT_H

#include "defines.h"
#include "logger.h"
#include "vp_client.h"

class afl_client{

    public:

        // The afl_client is a singleton. This reference is used for the signal handler.
        static afl_client* instance;

        // Creates a afl_client instance with its parameters.
        afl_client(int mode, int vp_instances, std::string vp_executable, std::string vp_launch_args, std::string target_path, int vp_loglevel, std::string vp_logging_path, int fksrv_st_fd, int fksrv_ctl_fd);

        // Static thread that watches a vp_client array and restarts them if needed.
        static void instance_restarter(int core_id, vp_client** clients_pointer, int clients_count);
        
        // Starts the forkserver client.
        void start(uint64_t mmio_address, std::string start_breakpoint, std::string end_breakpoint, std::string return_code_register,  int shm_cov_id, int shm_input_id);

        void shutdown();
        
        static void signal_handler(int sig);

    private:
        
        // Maximum of MAX_VP_INSTANCES instances.
        vp_client* m_vp_clients[MAX_VP_INSTANCES];

        int m_mode = 0;

        int m_vp_clients_count;
        int m_vp_clients_index = 0;

        std::string m_vp_executable;
        std::string m_vp_launch_args;
        std::string m_target_path;
        int m_vp_loglevel = 0;
        std::string m_vp_logging_path;
        int _profiler_count = 0;
        int m_fksrv_st_fd = -1;
        int m_fksrv_ctl_fd = -1;

        int m_shm_cov_id = -1;
        int m_shm_input_id = -1;

        static std::mutex restarter_mutex;

};

#endif
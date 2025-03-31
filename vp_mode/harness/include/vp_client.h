#ifndef VP_CLIENT_H
#define VP_CLIENT_H

#include "defines.h"
#include "logger.h"

// Client that interfaces with a the virtual platform process.
class vp_client{

    public:

        // States of the vp process.
        enum process_state{
            NOT_EXISTING, STARTING, STARTED, READY, DONE, KILLING, KILLED
        };

        // Structure of a fixed read.
        struct fixed_read{
            uint64_t address;
            char data;
        };

        // Structure of a interrupt trigger.
        struct interrupt_trigger{
            uint64_t interrupt_address;
            uint64_t trigger_address;
        };

        // Testing client from the vp-testing-interface for the current process
        testing::pipe_testing_client* vp_pipe_client;

        // TODO getter and setter.
        // State of the vp process that this client is communicating with.
        process_state vp_process_state = NOT_EXISTING;

        // TODO getter.
        // PID of the VP child process.
        pid_t vp_process = -1;
        
        bool retry_run = false;

        vp_client(std::string vp_executable, int vp_loglevel, std::string vp_logging_path, std::string vp_launch_args, std::string target_path, uint64_t mmio_start_address, uint64_t mmio_end_address);

        // Starting the VP in a new process.
        bool start_process();

        // Killing the current VP process.
        void kill_process();

        // Restarting the VP process.
        void restart_process(uint8_t fixed_read_count, fixed_read* fixed_reads, uint8_t interrupt_trigger_count, interrupt_trigger* interrupt_triggers, const char* error_symbol);

        // Waits for the VP to be ready.
        void waiting_for_ready();

        // Setups the VP for fuzzing with MMIO interception and code coverage tracking.
        void setup(uint8_t fixed_read_count, fixed_read* fixed_reads, uint8_t interrupt_trigger_count, interrupt_trigger* interrupt_triggers, const char* error_symbol);

        // Requests the VP to write the code coverage to a shared memory region.
        void write_code_coverage(int shm_id, unsigned int offset);

        // Requests one run of the VP with a test case.
        void do_run(uint64_t address, std::string start_breakpoint, std::string end_breakpoint, std::string return_register, int shm_id, unsigned int offset, int mmio_length);

        // Getting the return code from the VP. The return code was recoreded during the do_run function.
        void get_return_code();

        // Killing the VP
        void kill_vp();

        // Getter for the return value.
        uint64_t get_return_code_value();

    private:

        uint64_t m_ret_value = 0;

        std::string m_vp_executable;
        int m_vp_loglevel;
        std::string m_vp_logging_path;
        std::string m_vp_launch_args;
        std::string m_target_path;

        uint64_t m_mmio_start_address;
        uint64_t m_mmio_end_address;

};

#endif
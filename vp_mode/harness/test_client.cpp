#include <iostream>
#include <cstdlib>
#include <mqueue.h>
#include <cerrno>
#include <string.h>
#include <algorithm>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <fstream>
#include <cstdarg>
#include <csignal>
#include <cstring>
#include <sys/wait.h>
#include <unistd.h>
#include <sstream>
#include <ctime>
#include <vector>
#include <future>
#include <mutex>

// Import vp-testing-interface client.
#include "testing_client.h"

// Import profiler if used (can be set here or with INCLUDE_VP_HARNESS_PROFILER option).
#ifdef PROFILER_ENABLED
    #include <easy/profiler.h>
#else
    #define EASY_FUNCTION(...)
    #define EASY_BLOCK(...)
    #define EASY_END_BLOCK 
    #define EASY_PROFILER_ENABLE 
#endif

// Settings
#define PROFILING_COUNT 15
#define AFL_MODE
#define OWN_NAME "test_client"
#define MAX_VP_INSTANCES 20
// End Settings

// Data to enable shared memory fuzzing for AFLplusplus
#define FS_OPT_ENABLED 0x80000001
#define FS_OPT_SHDMEM_FUZZ 0x01000000

// Logging macro
#define LOG_MESSAGE(type, format, ...) logger::log(type, format, ##__VA_ARGS__)

// Custom logger to log into a file
class logger {

    public:

        enum log_level{
            DISABLED, ALL, WARNINGS_AND_ERRORS
        };

        enum log_type{
            INFO, WARNING, ERROR
        };

        static std::ofstream file_stream;
        static log_level selected_log_level;

        // Mutex to synchonize logging when multiple threads are logging to the same file.
        static std::mutex logger_mutex;

        // Initialize the logger with a filename
        static void init(const std::string& filename, log_level set_log_level) {
            file_stream.open(filename, std::ios::app); // Open for appending
            if (!file_stream.is_open()) {
                throw std::runtime_error("Unable to open log file: " + filename);
            }
            selected_log_level = set_log_level;
        }

        // Log a message
        static void log(log_type msg_log_type, const std::string& format, ...){
            va_list args;
            va_start(args, format);
            log(msg_log_type, format, args);
            va_end(args);
        }

        // Log a message
        static void log(log_type msg_log_type, const std::string& format, va_list args) {
            if (selected_log_level == DISABLED || (selected_log_level == WARNINGS_AND_ERRORS && (msg_log_type != WARNING && msg_log_type != ERROR))) return;
            if (file_stream.is_open()) {

                // Get current time
                char time_buffer[20]; // Buffer for timestamp [YYYY-MM-DD HH:MM:SS]
                std::time_t current_time = std::time(nullptr);
                std::strftime(time_buffer, sizeof(time_buffer), "%Y-%m-%d %H:%M:%S", std::localtime(&current_time));

                char messageBuffer[1024]; // Buffer for formatted message

                std::string log_type_string;

                switch(msg_log_type){
                    case INFO:
                        log_type_string = "INFO: ";
                    break;
                    case WARNING:
                        log_type_string = "WARNING: ";
                    break;
                    case ERROR:
                        log_type_string = "ERROR: ";
                    break;
                }

                // Format the message
                std::vsnprintf(messageBuffer, sizeof(messageBuffer), format.c_str(), args);

                // Output the timestamp and message to the file
                std::unique_lock<std::mutex> lock(logger_mutex);
                file_stream << "[" << time_buffer << "] " << log_type_string << messageBuffer << std::endl;
                lock.unlock();

            } else {
                std::cerr << "Log file is not open." << std::endl;
            }
        }

        static void log_error(const char* fmt, ...) {
            va_list args;
            va_start(args, fmt);
            log(log_type::ERROR, fmt, args);
            va_end(args);
        }

        static void log_info(const char* fmt, ...) {
            va_list args;
            va_start(args, fmt);
            log(log_type::INFO, fmt, args);
            va_end(args);
        }

        // Clean up the logger
        static void close() {
            if (file_stream.is_open()) {
                file_stream.close();
            }
        }
};

// Static variables of the logger
std::ofstream logger::file_stream;
logger::log_level logger::selected_log_level = logger::DISABLED;
std::mutex logger::logger_mutex;

// Client that interfaces with a the virtual platform process.
class vp_client{

    public:

        // States of the vp process.
        enum process_state{
            NOT_EXISTING, STARTING, STARTED, READY, DONE, KILLING, KILLED
        };

        // Testing client from the vp-testing-interface for the current process
        testing::pipe_testing_client* vp_pipe_client;

        // TODO getter and setter.
        // State of the vp process that this client is communicating with.
        process_state vp_process_state = NOT_EXISTING;

        // TODO getter.
        // PID of the VP child process.
        pid_t vp_process = -1;

        vp_client(std::string vp_executable, int vp_loglevel, std::string vp_logging_path, std::string vp_launch_args, std::string target_path, uint64_t mmio_start_address, uint64_t mmio_end_address){

            // Copy values to local variables.
            m_mmio_start_address = mmio_start_address;
            m_mmio_end_address = mmio_end_address;

            m_vp_executable = vp_executable;
            m_vp_loglevel = vp_loglevel;
            m_vp_logging_path = vp_logging_path;
            m_vp_launch_args = vp_launch_args;
            m_target_path = target_path;

            // Init a pipe client without specific file descriptors.
            vp_pipe_client = new testing::pipe_testing_client();

            // Callback for info and error logging. This enables the testing_client to also write into the logging file.
            vp_pipe_client->log_error_message = logger::log_error;
            vp_pipe_client->log_info_message = logger::log_info;

            // Start the communication.
            vp_pipe_client->start();
            
        };

        // Starting the VP in a new process.
        bool start_process(){
            EASY_FUNCTION(profiler::colors::Yellow);

            if(vp_process_state != NOT_EXISTING && vp_process != -1){
                LOG_MESSAGE(logger::ERROR, "Cannot start a VP process, because there is currently one running for this vp client: %d in state %d.", vp_process, (int)vp_process_state);  
                return false;
            }

            vp_process_state = STARTING;
            
            EASY_BLOCK("Start VP process");

                LOG_MESSAGE(logger::INFO, "Starting VP %s", m_vp_executable.c_str());

                // Create a child process.
                pid_t pid = fork();
                if (pid == 0) {
                    // Inside the child process.

                    std::string logErrorsOnlyArg = "";
                    
                    // Redirect stdout and stderr to files or /dev/null.
                    int logging;

                    // Create/open a logging file if logging is not disabled.
                    if(m_vp_loglevel > 0){
                        logging = open(m_vp_logging_path.c_str(), O_WRONLY | O_CREAT | O_APPEND, 0666);
                    }else{
                        logging = open("/dev/null", O_WRONLY);
                    }
                    
                    // Redirect output and error messages to the file or /dev/null (deleting them).
                    dup2(logging, STDOUT_FILENO);
                    dup2(logging, STDERR_FILENO);
                    close(logging);

                    // Args vector.
                    std::vector<char*> argv;

                    // Add the name of the executable to the first argument.
                    argv.push_back(strdup(m_vp_executable.c_str()));

                    // If log level is set to only errors, signal this to the VP.
                    if(m_vp_loglevel == 2){
                        std::string log_errors_only = "--log-errors-only";
                        argv.push_back(strdup(log_errors_only.c_str()));
                    }

                    // TODO into ENV ?
                    std::string full_launch_args = " --enable-test-receiver --test-receiver-interface 1 --test-receiver-pipe-request "+std::to_string(vp_pipe_client->get_request_fd())+" --test-receiver-pipe-response "+std::to_string(vp_pipe_client->get_response_fd())+" "+m_vp_launch_args;

                    // TODO 
                    std::istringstream iss(full_launch_args);
                    std::string token;
                    while (iss >> token) {
                        argv.push_back(strdup(token.c_str()));
                    }

                    argv.push_back(strdup(m_target_path.c_str()));
                    argv.push_back(nullptr);

                    // Execute the VP in this process.
                    execvp(argv[0], argv.data());
                    exit(127); // only if exec fails

                }

                // Parent process
                LOG_MESSAGE(logger::INFO, "Child process created: %d", (int)pid);

                vp_process = pid;
                vp_process_state = STARTED;

            EASY_END_BLOCK

            return vp_process > 0;
        }

        // Killing the current VP process.
        void kill_process(){
            EASY_FUNCTION(profiler::colors::Yellow);

            if(vp_process == -1){
                LOG_MESSAGE(logger::INFO, "Cannot kill the VP process, because there is none!");
                return;
            }

            vp_process_state = KILLING;

            LOG_MESSAGE(logger::INFO, "Killing VP process %d ...", vp_process);

            // Killing the VP child process
            kill(vp_process, SIGKILL);
            int status;
            waitpid(-1, &status, WNOHANG);

            vp_process = -1;
            vp_process_state = NOT_EXISTING;

            // Reset ready state of pipe client, because the VP process is not existing anymore. So waiting_for_ready needs to be called again for the new process.
            vp_pipe_client->reset_ready();

            LOG_MESSAGE(logger::INFO, "VP process killed.", vp_process);
        }

        // Restarting the VP process.
        void restart_process(){
            //kill_vp();
            // TODO wait ?
            kill_process();
            start_process();

            //TODO do not do bussy waiting !
            waiting_for_ready();
            setup();

            // TODO better logging: context of INSTANCE_RESTARTER!
            LOG_MESSAGE(logger::INFO, "Restart of VP process done!");
        }

        // Waits for the VP to be ready.
        void waiting_for_ready() {
            EASY_FUNCTION(profiler::colors::Red);

            EASY_BLOCK("Waiting for VP ready message");
                LOG_MESSAGE(logger::INFO, "Waiting for ready message.");
                //TODO check for error (return bool!) also with the others!
                vp_pipe_client->wait_for_ready();
            EASY_END_BLOCK
        }

        // Setups the VP for fuzzing with MMIO interception and code coverage tracking.
        void setup(){
            EASY_FUNCTION(profiler::colors::Magenta);

            LOG_MESSAGE(logger::INFO, "Setting up..");

            testing::request req = testing::request();
            testing::response res = testing::response();

            // Sends the ENABLE_CODE_COVERAGE command to the VP.
            EASY_BLOCK("Enable code coverage tracking");
                req.request_command = testing::ENABLE_CODE_COVERAGE;
                req.data_length = 0;
                vp_pipe_client->send_request(&req, &res);
            EASY_END_BLOCK

            // Sends the ENABLE_MMIO_TRACKING command to the VP with the start and end address specified in envs.
            EASY_BLOCK("Enable MMIO tracking");
                req.request_command = testing::ENABLE_MMIO_TRACKING;
                req.data_length = 17;
                req.data = (char*)malloc(req.data_length);
                testing::testing_communication::int64_to_bytes(m_mmio_start_address, req.data, 0);
                testing::testing_communication::int64_to_bytes(m_mmio_end_address, req.data, 8);
                // Sets the mode to only intercept read requests.
                req.data[16] = 1;
                vp_pipe_client->send_request(&req, &res);
            EASY_END_BLOCK

            // Freeing of req, res data not needed, because ther is none.

            LOG_MESSAGE(logger::INFO, "Setup done.");

            // Setting process state to ready to indicate this instance can be used for testing.
            vp_process_state = READY;
        }

        // Requests the VP to write the code coverage to a shared memory region.
        void write_code_coverage(int shm_id, unsigned int offset){
            EASY_FUNCTION(profiler::colors::Blue);

            LOG_MESSAGE(logger::INFO, "Request to write code coverage to: %d.", shm_id);

            testing::request req = testing::request();
            testing::response res = testing::response();

            // Sends the GET_CODE_COVERAGE_SHM command to the VP, which writes the code coverage to the shm_id with the offset.
            EASY_BLOCK("Writing code coverage");
                req.request_command = testing::GET_CODE_COVERAGE_SHM;
                req.data_length = 8;
                req.data = (char*)malloc(req.data_length);
                testing::testing_communication::int32_to_bytes((uint32_t)shm_id, req.data, 0);
                testing::testing_communication::int32_to_bytes((uint32_t)offset, req.data, 4);
                vp_pipe_client->send_request(&req, &res);
            EASY_END_BLOCK

            // Freeing req and res data.
            free(req.data);
            free(res.data);
        }

        // Requests one run of the VP with a test case.
        void do_run(uint64_t address, std::string start_breakpoint, std::string end_breakpoint, std::string return_register, int shm_id, unsigned int offset){
            EASY_FUNCTION(profiler::colors::Blue);
            
            LOG_MESSAGE(logger::INFO, "Requesting single run with start breakpoint %s to end breakpoint %s with MMIO data at %d.", start_breakpoint.c_str(), end_breakpoint.c_str(), shm_id);

            testing::request req = testing::request();
            testing::response res = testing::response();
            
            // Sends the DO_RUN_SHM request to the VP with the shared memory, return address and register and breakpoints.
            EASY_BLOCK("Requesting single run");
                req.request_command = testing::DO_RUN_SHM;

                req.data_length = 20+start_breakpoint.size()+end_breakpoint.size()+return_register.size();
                req.data = (char*)malloc(req.data_length);

                testing::testing_communication::int64_to_bytes(address, req.data, 0);
                testing::testing_communication::int32_to_bytes((uint32_t)shm_id, req.data, 8);
                testing::testing_communication::int32_to_bytes((uint32_t)offset, req.data, 12);
                // Stop reading the shared memory after string termination
                req.data[16] = 1;
                req.data[17] = start_breakpoint.size();
                req.data[18] = end_breakpoint.size();
                req.data[19] = return_register.size();
                strcpy(req.data+20, start_breakpoint.c_str());
                strcpy(req.data+20+start_breakpoint.size(), end_breakpoint.c_str());
                strcpy(req.data+20+start_breakpoint.size()+end_breakpoint.size(), return_register.c_str());

                vp_pipe_client->send_request(&req, &res);
            EASY_END_BLOCK

            // Freeing req data.
            free(req.data);
        }

        // Getting the return code from the VP. The return code was recoreded during the do_run function.
        void get_return_code(){
            EASY_FUNCTION(profiler::colors::Blue);

            testing::request req = testing::request();
            testing::response res = testing::response();

            // Sending command GET_RETURN_CODE to the VP.
            EASY_BLOCK("Getting return value");
                //Get exit status.
                req.request_command = testing::GET_RETURN_CODE;
                req.data_length = 0;
                vp_pipe_client->send_request(&req, &res);
                m_ret_value = testing::testing_communication::bytes_to_int64(res.data, 0);
                LOG_MESSAGE(logger::INFO, "Run done. Return code: %d", m_ret_value);
            EASY_END_BLOCK

            // Freeing res data.
            free(res.data);
        }

        // Killing the VP
        void kill_vp(){
            EASY_FUNCTION(profiler::colors::Magenta);

            LOG_MESSAGE(logger::INFO, "Killing VP ...");

            testing::request req = testing::request();
            testing::response res = testing::response();

            // Sending KILL command to the VP.
            EASY_BLOCK("Killing VP");
                //Kill
                req.request_command= testing::KILL;
                req.data_length = 1;
                req.data = (char*)malloc(1);
                //Killing the VP not gracefully.
                req.data[0] = 0;
                vp_pipe_client->send_request(&req, &res);
            EASY_END_BLOCK

            // Free request data.
            free(req.data);

            LOG_MESSAGE(logger::INFO, "VP Killed!");
        }

        // Getter for the return value.
        uint64_t get_return_code_value(){
            return m_ret_value;
        }

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

// Client that interfaces with AFLplusplus in vp-mode (-V).
class afl_client{

    public:

        // The afl_client is a singleton. This reference is used for the signal handler.
        static afl_client* instance;

        // Creates a afl_client instance with its parameters.
        afl_client(int mode, int vp_instances, std::string vp_executable, std::string vp_launch_args, std::string target_path, int vp_loglevel, std::string vp_logging_path, int fksrv_st_fd, int fksrv_ctl_fd){
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

        // Static thread that watches a vp_client array and restarts them if needed.
        static void instance_restarter(int core_id, vp_client** clients_pointer, int clients_count){

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
        
        // Starts the forkserver client.
        void start(uint64_t mmio_address, std::string start_breakpoint, std::string end_breakpoint, std::string return_code_register,  int shm_cov_id, int shm_input_id) {

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

        void shutdown(){
            //TODO SIGTERM or SIGKILL ?
            for(int i=0; i<m_vp_clients_count; i++){
                m_vp_clients[i]->kill_process();
            }
        }

        static void signal_handler(){
            signal_handler(15);
        }

        static void signal_handler(int sig) {
            // Handle SIGTERM or other signals as needed
            LOG_MESSAGE(logger::ERROR, "Terminating on signal %d", sig);

            // killing current vp instance
            if(instance != nullptr){
                instance->shutdown();
            }

            exit(0);
        }

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

afl_client* afl_client::instance = nullptr;
std::mutex afl_client::restarter_mutex;

int main(int argc, char* argv[]) {
    // Start profiler
    EASY_PROFILER_ENABLE;

    // AFL mode runs the forkserver with shared memory
    #ifdef AFL_MODE

        EASY_BLOCK("Setup");

            //Enable logging if env is set (TC_LOGGING="1" and TC_LOGGING_PATH to a path)
            const char* logging = std::getenv("TC_LOGGING");
            const char* logging_path = std::getenv("TC_LOGGING_PATH");
            int logLevel = 0;

            if(logging != nullptr){

                try{
                    logLevel = std::stoi(logging);
                }catch(std::exception &e){
                    //TODO log with println here ?
                    return 1;
                }

                if(logLevel > 0 && logging_path){
                    logger::init(logging_path, (logger::log_level)logLevel);
                    LOG_MESSAGE(logger::WARNING, "LOGGING ENABLED!");
                }
            }
            
            // Check required parameters
            if (argc != 6) {
                LOG_MESSAGE(logger::ERROR, "Wrong parameters!");
                return 1;
            }

            const char* target_path = argv[1];
            LOG_MESSAGE(logger::INFO, "Target program path: %s", target_path);

            int shm_input = 0;
            try{
                shm_input = std::stoi(argv[2]);
                LOG_MESSAGE(logger::INFO, "Shared memory ID for test cases: %d", shm_input);
            }catch(std::exception &e){
                LOG_MESSAGE(logger::INFO, "Argument test case shared memory is not a valid number!");
                return 1;
            }

            int shm_cov = 0;
            try{
                shm_cov = std::stoi(argv[3]);
                LOG_MESSAGE(logger::INFO, "Shared memory ID for code coverage: %d", shm_cov);
            }catch(std::exception &e){
                LOG_MESSAGE(logger::INFO, "Argument coverage shared memory is not a valid number!");
                return 1;
            }

            int ctl_fd = 0;
            try{
                ctl_fd = std::stoi(argv[4]);
                LOG_MESSAGE(logger::INFO, "Forkserver control FD: %d", ctl_fd);
            }catch(std::exception &e){
                LOG_MESSAGE(logger::INFO, "Argument for forkserver control FD is not a valid number!");
                return 1;
            }

            int st_fd = 0;
            try{
                st_fd = std::stoi(argv[5]);
                LOG_MESSAGE(logger::INFO, "Forkserver status FD: %d", st_fd);
            }catch(std::exception &e){
                LOG_MESSAGE(logger::INFO, "Argument for forkserver status FD is not a valid number!");
                return 1;
            }

            //Check settings from environment variables
            const char* vp_executable = std::getenv("TC_VP_EXECUTABLE");
            if(vp_executable){
                LOG_MESSAGE(logger::INFO, "VP executable (TC_VP_EXECUTABLE): %s", vp_executable);
            }else{
                LOG_MESSAGE(logger::ERROR, "TC_VP_EXECUTABLE envirnoment variable not set!");
                return 1;
            }

            const char* vp_launch_args = std::getenv("TC_VP_LAUNCH_ARGS");
            if(vp_launch_args){
                LOG_MESSAGE(logger::INFO, "VP launch args (TC_VP_LAUNCH_ARGS): %s", vp_launch_args);
            }else{
                vp_launch_args = "";
            }

            const char* vp_logging = std::getenv("TC_VP_LOGGING");
            const char* vp_logging_path = std::getenv("TC_VP_LOGGING_PATH");
            int vp_loglevel = 0;
 
            if(vp_logging){

                try{
                    vp_loglevel = std::stoi(vp_logging);
                }catch(std::exception &e){
                    LOG_MESSAGE(logger::ERROR, "Could not parse value of TC_VP_LOGGING! Logging disabled.");
                }

                if(vp_loglevel > 0 && logging_path){
                    LOG_MESSAGE(logger::INFO, "VP logging path (TC_VP_LOGGING_PATH): %s", vp_logging_path);
                }else{
                    LOG_MESSAGE(logger::ERROR, "TC_VP_LOGGING_PATH envirnoment variable not set, but TC_VP_LOGGING enabled! Disabling VP logging.");
                    vp_loglevel = 0;
                    vp_logging_path = "";
                }

            }else{
                vp_logging_path = "";
            }

            const char* kill_old = std::getenv("TC_KILL_OLD");
            if(kill_old && strcmp(kill_old, "1") == 0){
                LOG_MESSAGE(logger::INFO, "Killing old processes (TC_KILL_OLD) enabled.");
                
                std::string command = "killall "+std::string(vp_executable);
                int ret = system(command.c_str());
                if (ret == -1) LOG_MESSAGE(logger::ERROR, "Error occoured while trying to killall %d. Continuing.", vp_executable);

                ret = system("killall --older-than 5s " OWN_NAME);
                if (ret == -1) LOG_MESSAGE(logger::ERROR, "Error occoured while trying to killall %d. Continuing.", vp_executable);
            }

            //TODO check after killing! Or do it multiple times
            //TODO fedback to afl about errors

            const char* mode_str = std::getenv("TC_MODE");
            int mode = 0;
            if(mode_str){
                try{
                    mode = std::stoi(mode_str);
                    LOG_MESSAGE(logger::INFO, "Test client mode (TC_MODE) set to: %d", mode);
                }catch(std::exception &e){
                    LOG_MESSAGE(logger::ERROR, "Could not parse value of TC_MODE!");
                    return 1;
                }
            }else{
                LOG_MESSAGE(logger::ERROR, "TC_MODE envirnoment variable not set, but is required!");
                return 1;
            }

            const char* vp_instances_str = std::getenv("TC_VP_INSTANCES");
            int vp_instances = 1;
            if(vp_instances_str){
                try{
                    vp_instances = std::stoi(vp_instances_str);
                    LOG_MESSAGE(logger::INFO, "Number of VP instances (TC_VP_INSTANCES) set to: %d", vp_instances);
                }catch(std::exception &e){
                    LOG_MESSAGE(logger::ERROR, "Could not parse value of TC_VP_INSTANCES! Set to defaut: 1.");
                    vp_instances = 1;
                }
            }

            const char* start_symbol = std::getenv("TC_START_SYMBOL");
            if(start_symbol){
                LOG_MESSAGE(logger::INFO, "Start symbol (TC_START_SYMBOL): %s", start_symbol);
            }else{
                LOG_MESSAGE(logger::ERROR, "TC_START_SYMBOL envirnoment variable not set!");
                return 1;
            }

            const char* end_symbol = std::getenv("TC_END_SYMBOL");
            if(end_symbol){
                LOG_MESSAGE(logger::INFO, "End symbol (TC_END_SYMBOL): %s", end_symbol);
            }else{
                LOG_MESSAGE(logger::ERROR, "TC_END_SYMBOL envirnoment variable not set!");
                return 1;
            }

            const char* return_register = std::getenv("TC_RETURN_REGISTER");
            if(return_register){
                LOG_MESSAGE(logger::INFO, "Return register (TC_RETURN_REGISTER): %s", return_register);
            }else{
                LOG_MESSAGE(logger::ERROR, "TC_RETURN_REGISTER envirnoment variable not set!");
                return 1;
            }

            const char* mmio_data_address_str = std::getenv("TC_MMIO_DATA_ADDRESS");
            int mmio_data_address = 0;
            if(mmio_data_address_str){
                try{
                    mmio_data_address = std::stoul(mmio_data_address_str, nullptr, 16);
                    LOG_MESSAGE(logger::INFO, "MMIO data address (TC_MMIO_DATA_ADDRESS) set to: %d", mmio_data_address);
                }catch(std::exception &e){
                    LOG_MESSAGE(logger::ERROR, "Could not parse value of TC_MMIO_DATA_ADDRESS!");
                    return 1;
                }
            }else{
                LOG_MESSAGE(logger::ERROR, "TC_MMIO_DATA_ADDRESS envirnoment variable not set, but is required!");
                return 1;
            }

            afl_client m_afl_client = afl_client(mode, vp_instances, vp_executable, vp_launch_args, target_path, vp_loglevel, vp_logging_path, st_fd, ctl_fd);

            // Set termination signal handler
            std::signal(SIGTERM, afl_client::signal_handler);

        EASY_END_BLOCK
        

        m_afl_client.start(mmio_data_address, start_symbol, end_symbol, return_register, shm_cov, shm_input);

    // Not AFL mode just does one simple run in avp64
    #else

        logger::init("tc_out.txt", logger::ALL);

        LOG_MESSAGE(logger::INFO, "Run manual.");

        vp_client vp_client = vp_client();
        vp_client.waitingForReady();
        vp_client.setup();
        
        //vp_client.run("pasw", true);
        //vp_client.run("pass", false);
        //vp_client.kill();

        //TODO fix (with shared memory)
        vp_client.run_single("main", "exit", "pass");
        vp_client.write_code_coverage(12345);
        vp_client.kill();

        #ifdef PROFILER_ENABLED
            LOG_MESSAGE(logger::INFO, "Writing profiling file.");
            profiler::dumpBlocksToFile("test_client.prof");
        #endif
    
    #endif

    return 0;
}
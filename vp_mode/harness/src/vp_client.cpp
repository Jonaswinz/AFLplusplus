#include "vp_client.h"

vp_client::vp_client(std::string vp_executable, int vp_loglevel, std::string vp_logging_path, std::string vp_launch_args, std::string target_path, uint64_t mmio_start_address, uint64_t mmio_end_address){

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

bool vp_client::start_process(){
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

void vp_client::kill_process(){
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

void vp_client::restart_process(){
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

void vp_client::waiting_for_ready() {
    EASY_FUNCTION(profiler::colors::Red);

    EASY_BLOCK("Waiting for VP ready message");
        LOG_MESSAGE(logger::INFO, "Waiting for ready message.");
        //TODO check for error (return bool!) also with the others!
        vp_pipe_client->wait_for_ready();
    EASY_END_BLOCK
}

void vp_client::setup(){
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

void vp_client::write_code_coverage(int shm_id, unsigned int offset){
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

void vp_client::do_run(uint64_t address, std::string start_breakpoint, std::string end_breakpoint, std::string return_register, int shm_id, unsigned int offset){
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

void vp_client::get_return_code(){
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

void vp_client::kill_vp(){
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

uint64_t vp_client::get_return_code_value(){
    return m_ret_value;
}

    
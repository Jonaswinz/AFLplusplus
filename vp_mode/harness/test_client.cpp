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


#ifdef PROFILER_ENABLED
    #include <easy/profiler.h>
#else
    #define EASY_FUNCTION(...)
    #define EASY_BLOCK(...)
    #define EASY_END_BLOCK 
    #define EASY_PROFILER_ENABLE 
#endif

// Settings
#define PROFILING_COUNT 5
#define AFL_MODE
#define OWN_NAME "test_client"
// End Settings

#define REQUEST_LENGTH 256
#define RESPONSE_LENGTH 256

#define REQUEST_READ_FD 10
#define RESPONSE_WRITE_FD 11

#define MAP_SIZE_POW2 16
#define MAP_SIZE (1 << MAP_SIZE_POW2)

#define FS_OPT_ENABLED 0x80000001
#define FS_OPT_SHDMEM_FUZZ 0x01000000

#define LOG_MESSAGE(type, format, ...) Logger::log(type, format, ##__VA_ARGS__)

class Logger {
public:

    enum LogLevel{
        DISABLED, ALL, WARNINGS_AND_ERRORS
    };

    enum LogType{
        INFO, WARNING, ERROR
    };

    static std::ofstream fileStream;
    static LogLevel logLevel;

    // Initialize the logger with a filename
    static void init(const std::string& filename, LogLevel setLogLevel) {
        fileStream.open(filename, std::ios::app); // Open for appending
        if (!fileStream.is_open()) {
            throw std::runtime_error("Unable to open log file: " + filename);
        }
        logLevel = setLogLevel;
    }

    // Log a message
    static void log(LogType logType, const std::string& format, ...) {
        if (logLevel == DISABLED || (logLevel == WARNINGS_AND_ERRORS && (logType != WARNING && logType != ERROR))) return;
        if (fileStream.is_open()) {

            // Get current time
            char timeBuffer[20]; // Buffer for timestamp [YYYY-MM-DD HH:MM:SS]
            std::time_t current_time = std::time(nullptr);
            std::strftime(timeBuffer, sizeof(timeBuffer), "%Y-%m-%d %H:%M:%S", std::localtime(&current_time));

            char messageBuffer[1024]; // Buffer for formatted message

            std::string logTypeString;

            switch(logType){
                case INFO:
                    logTypeString = "INFO: ";
                break;
                case WARNING:
                    logTypeString = "WARNING: ";
                break;
                case ERROR:
                    logTypeString = "ERROR: ";
                break;
            }

            // Format the message
            va_list args;
            va_start(args, format);
            std::vsnprintf(messageBuffer, sizeof(messageBuffer), format.c_str(), args);
            va_end(args);

            // Output the timestamp and message to the file
            fileStream << "[" << timeBuffer << "] " << logTypeString << messageBuffer << std::endl;

        } else {
            std::cerr << "Log file is not open." << std::endl;
        }
    }

    // Clean up the logger
    static void close() {
        if (fileStream.is_open()) {
            fileStream.close();
        }
    }

};

std::ofstream Logger::fileStream;
Logger::LogLevel Logger::logLevel = Logger::DISABLED;

enum Command{
    CONTINUE, KILL, SET_BREAKPOINT, REMOVE_BREAKPOINT, SET_MMIO_TRACKING, DISABLE_MMIO_TRACKING, SET_MMIO_VALUE, SET_CODE_COVERAGE, REMOVE_CODE_COVERAGE, GET_CODE_COVERAGE, GET_EXIT_STATUS, RESET_CODE_COVERAGE, DO_RUN, WRITE_CODE_COVERAGE
};

enum Status {
    STATUS_OK, MMIO_READ, MMIO_WRITE, VP_END, BREAKPOINT_HIT, ERROR=-1
};

struct Request{
    Command command;
    char data[REQUEST_LENGTH-1];
    size_t dataLength = 0;
};

struct Response{
    char data[RESPONSE_LENGTH];
    size_t dataLength = 0;
};

class VPClient{

    public:

        int bb_list_size = 0;
        char bb_list [MAP_SIZE];
        int fd_request[2], fd_response[2];

        VPClient(){

            if (pipe(fd_request) == -1 || pipe(fd_response) == -1) {
                LOG_MESSAGE(Logger::ERROR, "Error creating new pipes!");
                exit(1);
            }

            LOG_MESSAGE(Logger::INFO, "Created communication pipes. Request FD: %d,%d and resonse FD: %d,%d", fd_request[0], fd_request[1], fd_response[0], fd_response[1]);

            if(dup2(fd_request[0], REQUEST_READ_FD)  == -1 || dup2(fd_response[1], RESPONSE_WRITE_FD) == -1){
                LOG_MESSAGE(Logger::ERROR, "Error setting file descriptor of request read to %d and response write to %d pipes.", REQUEST_READ_FD, RESPONSE_WRITE_FD);
                exit(1);
            }

            LOG_MESSAGE(Logger::INFO, "Setting pipe file descriptor of request read to %d and response write to %d.", REQUEST_READ_FD, RESPONSE_WRITE_FD);

            //TODO close pipes etc.
            
        };

        void waitingForReady() {
            EASY_FUNCTION(profiler::colors::Red);
            EASY_BLOCK("Waiting for VP ready message");
                LOG_MESSAGE(Logger::INFO, "Waiting for ready message.");

                char* buffer = new char[RESPONSE_LENGTH];
                unsigned int priority;

                while (true) {
                    ssize_t bytes_read = read(fd_response[0], buffer, RESPONSE_LENGTH);

                    if (bytes_read == -1) {
                        LOG_MESSAGE(Logger::ERROR, "ERROR: An error occurred while waiting for ready message: %s", strerror(errno));
                        break;
                    }

                    buffer[bytes_read] = '\0'; // Ensure null-termination for valid C-string
                    std::string message(buffer);
                    if (message == "ready") {
                        LOG_MESSAGE(Logger::INFO, "Received ready message!");
                        break;
                    }
                }

                delete[] buffer;
            EASY_END_BLOCK
        }

        bool sendRequest(Request* request, Response* response) {

            size_t send_length = request->dataLength+1;

            if (write(fd_request[1], &send_length, sizeof(send_length)) == -1) {
                LOG_MESSAGE(Logger::ERROR, "Error sending message length %s", strerror(errno));
                return false;
            }

            if (write(fd_request[1], &request->command, sizeof(request->command)) == -1) {
                LOG_MESSAGE(Logger::ERROR, "Error sending message command %s", strerror(errno));
                return false;
            }

            if (write(fd_request[1], request->data, request->dataLength) == -1) {
                LOG_MESSAGE(Logger::ERROR, "Error sending message data %s", strerror(errno));
                return false;
            }

            LOG_MESSAGE(Logger::INFO, "SENT: %d", request->command);

            ssize_t bytes_read = read(fd_response[0], &response->dataLength, sizeof(response->dataLength));
            if (bytes_read != sizeof(response->dataLength)) {
                LOG_MESSAGE(Logger::ERROR, "Error receiving response data length: %s", strerror(errno));
                return false;
            }

            if(response->dataLength > 0){
                bytes_read = read(fd_response[0], response->data, response->dataLength);
                if (bytes_read == -1) {
                    LOG_MESSAGE(Logger::ERROR, "Error receiving response data: %s", strerror(errno));
                    return false;
                }
            }

            return true;
        }   

        void setup(){
            EASY_FUNCTION(profiler::colors::Magenta);
        
            LOG_MESSAGE(Logger::INFO, "Setting up..");

            Request request = Request();
            Response response = Response();

            EASY_BLOCK("Enable code coverage tracking");
                //Enable code coverage tracking.
                request.command = SET_CODE_COVERAGE;
                request.dataLength = 0;
                sendRequest(&request, &response);
            EASY_END_BLOCK

            EASY_BLOCK("Enable MMIO tracking");
                //Enable MMIO tracking.
                request.command = SET_MMIO_TRACKING;
                request.dataLength = 0;
                sendRequest(&request, &response);
            EASY_END_BLOCK

            LOG_MESSAGE(Logger::INFO, "Setup done.");

        }

        void write_code_coverage(int shm_id, unsigned int offset){

            LOG_MESSAGE(Logger::INFO, "Request to write code coverage to: %d.", shm_id);

            EASY_BLOCK("Writing code coverage");

                Request request = Request();
                Response response = Response();

                request.command = WRITE_CODE_COVERAGE;
                request.data[0] = (char)(shm_id & 0xFF);
                request.data[1] = (char)((shm_id >> 8) & 0xFF);
                request.data[2] = (char)((shm_id >> 16) & 0xFF);
                request.data[3] = (char)((shm_id >> 24) & 0xFF);
                request.data[4] = (char)(offset & 0xFF);
                request.data[5] = (char)((offset >> 8) & 0xFF);
                request.data[6] = (char)((offset >> 16) & 0xFF);
                request.data[7] = (char)((offset >> 24) & 0xFF);
                request.dataLength = 8;

                sendRequest(&request, &response);

            EASY_END_BLOCK
        }

        void run_single(std::string start_breakpoint, std::string end_breakpoint, int shm_id, unsigned int offset){
            EASY_FUNCTION(profiler::colors::Magenta);
            
            LOG_MESSAGE(Logger::INFO, "Requesting single run with start breakpoint %s to end breakpoint %s with MMIO data at %d.", start_breakpoint.c_str(), end_breakpoint.c_str(), shm_id);

            Request request = Request();
            Response response = Response();

            EASY_BLOCK("Requesting single run");
                request.command = DO_RUN;
                request.data[0] = start_breakpoint.size();
                strcpy(request.data+1, start_breakpoint.c_str());
                request.data[1+start_breakpoint.size()] = end_breakpoint.size();
                strcpy(request.data+2+start_breakpoint.size(), end_breakpoint.c_str());

                request.data[2+start_breakpoint.size()+end_breakpoint.size()] = (char)(shm_id & 0xFF);
                request.data[3+start_breakpoint.size()+end_breakpoint.size()] = (char)((shm_id >> 8) & 0xFF);
                request.data[4+start_breakpoint.size()+end_breakpoint.size()] = (char)((shm_id >> 16) & 0xFF);
                request.data[5+start_breakpoint.size()+end_breakpoint.size()] = (char)((shm_id >> 24) & 0xFF);

                request.data[6+start_breakpoint.size()+end_breakpoint.size()] = (char)(offset & 0xFF);
                request.data[7+start_breakpoint.size()+end_breakpoint.size()] = (char)((offset >> 8) & 0xFF);
                request.data[8+start_breakpoint.size()+end_breakpoint.size()] = (char)((offset >> 16) & 0xFF);
                request.data[9+start_breakpoint.size()+end_breakpoint.size()] = (char)((offset >> 24) & 0xFF);

                request.dataLength = 10+start_breakpoint.size()+end_breakpoint.size();

                sendRequest(&request, &response);
            EASY_END_BLOCK

            //TODO put in own function:
            EASY_BLOCK("Getting exit value");
                //Get exit status.
                request.command = GET_EXIT_STATUS;
                request.dataLength = 0;
                sendRequest(&request, &response);
                ret_value = response.data[0];
                LOG_MESSAGE(Logger::INFO, "Run done. Exit code: %d", ret_value);
            EASY_END_BLOCK

            /*
            EASY_BLOCK("Getting code coverage");
                //Get code coverage.
                achtung das muss noch geändert werden! also data 0 muss zu command
                request.data[0] = GET_CODE_COVERAGE;
                request.dataLength = 1;
                sendRequest(&request, &response);
                int converage_length = (response.data[0]) | (response.data[1] << 8) | (response.data[2] << 16) | (response.data[3] << 24);
                LOG_MESSAGE(Logger::INFO, "Coverage length: %d", converage_length);

                bb_list_size = converage_length;

                int received_length = 0;
                while(received_length < converage_length) {
                    size_t next_chunk_size = std::min(RESPONSE_LENGTH, converage_length-received_length);
                    ssize_t bytes_read = mq_receive(mqt_responses, bb_list+received_length, next_chunk_size, NULL);
                    received_length += next_chunk_size;
                }
                LOG_MESSAGE(Logger::INFO, "Coverage received!");
            EASY_END_BLOCK  
            */
        
        }

        void run(std::string MMIO_data, bool continueRun){
            EASY_FUNCTION(profiler::colors::Magenta);
            
            LOG_MESSAGE(Logger::INFO, "Running ...");

            size_t MMIO_data_index = 0;

            Request request = Request();
            Response response = Response();

            //TODO set via env

            EASY_BLOCK("Setting exit breakpoint");
                //Set exit breakpoint.
                request.command = SET_BREAKPOINT;
                request.data[0] = 0;
                strcpy(request.data+1, "exit");
                request.dataLength = 5;
                sendRequest(&request, &response);
            EASY_END_BLOCK


            while(true){

                if(response.data[0] == VP_END){
                    LOG_MESSAGE(Logger::INFO, "Received VP_END.");
                    break;
                }else if(response.data[0] == MMIO_READ){
                    LOG_MESSAGE(Logger::INFO, "Received MMIO_READ.");

                    if(MMIO_data_index < MMIO_data.size()+1){
                        
                        LOG_MESSAGE(Logger::INFO, "Sending: %c", MMIO_data.c_str()[MMIO_data_index]);

                        EASY_BLOCK("Sending MMIO value");
                            //Set MMIO character
                            request.command = SET_MMIO_VALUE;
                            request.data[0] = 1;
                            request.data[1] = MMIO_data.c_str()[MMIO_data_index];
                            request.dataLength = 2;
                            sendRequest(&request, &response);
                            MMIO_data_index ++;
                        EASY_END_BLOCK

                    }else{
                        LOG_MESSAGE(Logger::ERROR, "More data requested!");
                    }

                }else if(response.data[0] == BREAKPOINT_HIT){
                    LOG_MESSAGE(Logger::INFO, "Received exit breakpoint hit.");
                    break;
                }else{
                    LOG_MESSAGE(Logger::INFO, "Continuing: Other");

                    EASY_BLOCK("Sending Continue");
                        //Continue
                        request.command = CONTINUE;
                        request.dataLength = 0;
                        sendRequest(&request, &response);
                    EASY_END_BLOCK
                }
            }

            EASY_BLOCK("Getting exit value");
                //Get exit status.
                request.command = GET_EXIT_STATUS;
                request.dataLength = 0;
                sendRequest(&request, &response);
                ret_value = response.data[0];
                LOG_MESSAGE(Logger::INFO, "Exit code: %d", ret_value);
            EASY_END_BLOCK

            /*
            EASY_BLOCK("Getting code coverage");
                //Get code coverage.
                achtung das muss noch geändert werden! also data 0 muss zu command
                request.data[0] = GET_CODE_COVERAGE;
                request.dataLength = 1;
                sendRequest(&request, &response);
                int converage_length = (response.data[0]) | (response.data[1] << 8) | (response.data[2] << 16) | (response.data[3] << 24);
                LOG_MESSAGE(Logger::INFO, "Coverage length: %d", converage_length);

                bb_list_size = converage_length;

                int received_length = 0;
                while(received_length < converage_length) {
                    size_t next_chunk_size = std::min(RESPONSE_LENGTH, converage_length-received_length);
                    ssize_t bytes_read = mq_receive(mqt_responses, bb_list+received_length, next_chunk_size, NULL);
                    received_length += next_chunk_size;
                }
                LOG_MESSAGE(Logger::INFO, "Coverage received!");
            EASY_END_BLOCK 
            */ 
            

            if(continueRun){
                //TODO differently

                LOG_MESSAGE(Logger::INFO, "Resetting run.");

                EASY_BLOCK("Resetting code coverage");
                    //Set exit breakpoint.
                    request.command = RESET_CODE_COVERAGE;
                    request.dataLength = 0;
                    sendRequest(&request, &response);
                EASY_END_BLOCK

                //TODO set dynamically

                EASY_BLOCK("Setting _start breakpoint");
                    //Set exit breakpoint.
                    request.command = SET_BREAKPOINT;
                    request.data[0] = 0;
                    strcpy(request.data+1, "main");
                    request.dataLength = 5;
                    sendRequest(&request, &response);
                EASY_END_BLOCK

                while(true){

                    if(response.data[0] == BREAKPOINT_HIT){
                        LOG_MESSAGE(Logger::INFO, "Received main breakpoint hit.");
                        break;
                    }else{
                        LOG_MESSAGE(Logger::INFO, "Continuing: Other");

                        EASY_BLOCK("Sending Continue");
                            //Continue
                            request.command = CONTINUE;
                            request.dataLength = 0;
                            sendRequest(&request, &response);
                        EASY_END_BLOCK
                    }

                }
            }

        }

        void kill(){
            EASY_FUNCTION(profiler::colors::Magenta);

            LOG_MESSAGE(Logger::INFO, "Killing ...");

            Request request = Request();
            Response response = Response();

            EASY_BLOCK("killing");
                //Kill
                request.command = KILL;
                request.dataLength = 0;
                sendRequest(&request, &response);
            EASY_END_BLOCK

            LOG_MESSAGE(Logger::INFO, "Killed!");
        }

        int getRetValue(){
            return ret_value;
        }

    private:

        int ret_value = 0;

};

//TODO: schreiben das singleton ist weil wegen static instance, benötigt für static signal handler
class AFLForkserver{

    public:

        static AFLForkserver* instance;
    
        AFLForkserver(VPClient* vp_client, std::string vp_executable, std::string vp_launch_args, std::string target_path, int vp_loglevel, std::string vp_logging_path, int fksrv_st_fd, int fksrv_ctl_fd){
            _vp_client = vp_client;
            _vp_executable = vp_executable;
            _vp_launch_args = vp_launch_args;
            _target_path = target_path;
            _vp_loglevel = vp_loglevel;
            _vp_logging_path = vp_logging_path;
            _fksrv_st_fd = fksrv_st_fd;
            _fksrv_ctl_fd = fksrv_ctl_fd;

            instance = this;
        };

        std::string readFromTestcaseSharedMemory() {
            
            //Attaching shared memory if not attached yet.
            if(_shm_input_data == nullptr){
                _shm_input_data = static_cast<char*>(shmat(_shm_input_id, nullptr, SHM_RDONLY));
                if (_shm_input_data == reinterpret_cast<char*>(-1)) {
                    LOG_MESSAGE(Logger::ERROR, "Failed to attach test case shared memory segment: %s", strerror(errno));
                    _shm_input_data = nullptr;
                    return "";
                }

                //the fuzzing input actually starts after this uint32_t.
                _shm_input_data += sizeof(uint32_t);
            }

            
            struct shmid_ds shm_info;
            if (shmctl(_shm_input_id, IPC_STAT, &shm_info) == -1) {
                LOG_MESSAGE(Logger::ERROR, "Reading length of shared memory of id %d.", _shm_input_id);
                return "";
            }

            std::string readData;

            //If there is not a termination character at the end, copy the data and change the last character to one. This is necessary, because the shared memory is only opnened in ready-only mode.
            if(_shm_input_data[(size_t)shm_info.shm_segsz] != 0){

                LOG_MESSAGE(Logger::WARNING, "Last character of test case was not a termination character!");
                
                char* temp = (char*)malloc((size_t)shm_info.shm_segsz);
                memcpy(temp, _shm_input_data, (size_t)shm_info.shm_segsz);
                temp[(size_t)shm_info.shm_segsz] = 0;
                readData = std::string(temp);
                free(temp);

            }else{
                // Copy the data to a std::string to handle automatic memory management
                readData = std::string(_shm_input_data);
            }
            
            return readData;
        }

        bool writeToCoverageSharedMemory(int shm_id, const char* data, size_t size) {

            //Attaching shared memory if not attached yet.
            if(_shm_cov_data == nullptr){
                _shm_cov_data = static_cast<char*>(shmat(_shm_cov_id, nullptr, 0));
                if (_shm_cov_data == reinterpret_cast<char*>(-1)) {
                    LOG_MESSAGE(Logger::ERROR, "Failed to attach test case shared memory segment: %s", strerror(errno));
                    _shm_cov_data = nullptr;
                    return false;
                }
            }

            struct shmid_ds shm_info;
            if (shmctl(_shm_cov_id, IPC_STAT, &shm_info) == -1) {
                LOG_MESSAGE(Logger::ERROR, "Reading length of shared memory of id %d.", _shm_cov_id);
                return "";
            }

            if(size > (size_t)shm_info.shm_segsz){
                LOG_MESSAGE(Logger::ERROR, "Size of the data %d is larger then the shared memory segment (%d) .", size, (int)shm_info.shm_segsz);
                return false;
            }

            // Write the data to the shared memory
            std::memcpy(_shm_cov_data, data, shm_info.shm_segsz);

            return true;
        }

        pid_t start_vp(){
            EASY_FUNCTION(profiler::colors::Blue);
            EASY_BLOCK("Start VP process");

                std::string command = _vp_executable+" "+_vp_launch_args+" "+_target_path;
                LOG_MESSAGE(Logger::INFO, "Starting VP with command: %s", command.c_str());

                pid_t pid = fork();
                if (pid == 0) { // Child process

                    // Redirect stdout and stderr to files or /dev/null
                    int logging;

                    std::string logErrorsOnlyArg = "";

                    if(_vp_loglevel > 0){
                        logging = open(_vp_logging_path.c_str(), O_WRONLY | O_CREAT | O_APPEND, 0666);
                    }else{
                        logging = open("/dev/null", O_WRONLY);
                    }

                    dup2(logging, STDOUT_FILENO);
                    dup2(logging, STDERR_FILENO);
                    close(logging);

                    //TODO splie args into multiple
                    //execl("/bin/sh", "sh", "-c", command.c_str(), (char*)NULL);

                    std::vector<char*> argv;
                    argv.push_back(strdup(_vp_executable.c_str()));

                    if(_vp_loglevel == 2){
                        std::string log_errors_only = "--log-errors-only";
                        argv.push_back(strdup(log_errors_only.c_str()));
                    }

                    std::istringstream iss(_vp_launch_args);
                    std::string token;
                    while (iss >> token) {
                        argv.push_back(strdup(token.c_str()));
                    }

                    argv.push_back(strdup(_target_path.c_str()));
                    argv.push_back(nullptr);


                    execvp(argv[0], argv.data());
                    exit(127); // only if exec fails

                } else if (pid < 0) {
                    LOG_MESSAGE(Logger::ERROR, "Failed to fork process.");
                    shotdown();
                    exit(1);
                }

                // Parent process
                LOG_MESSAGE(Logger::INFO, "Child process created: %d", (int)pid);

                return pid;

            EASY_END_BLOCK

            return -1;
        }
        
        void fksrv_start(int shm_cov_id, int shm_input_id, bool restart) {

            _shm_cov_id = shm_cov_id;
            _shm_input_id = shm_input_id;

            // Communicate initial status
            EASY_BLOCK("Communicate initial status");
                int status = FS_OPT_ENABLED | FS_OPT_SHDMEM_FUZZ;
                if (write(_fksrv_st_fd, &status, sizeof(status)) != sizeof(status)) {
                    LOG_MESSAGE(Logger::ERROR, "Not running in forkserver mode, just executing the program.");
                }
            EASY_END_BLOCK

            // Read response from AFL
            EASY_BLOCK("Read response from AFL");
                int read_status;
                if (read(_fksrv_ctl_fd, &read_status, sizeof(read_status)) != sizeof(read_status)) {
                    LOG_MESSAGE(Logger::ERROR, "AFL parent exited before forkserver was up.");
                    shotdown();
                    exit(1);
                } else if (read_status != status) {
                    LOG_MESSAGE(Logger::INFO, "Read response from AFL: %d need %d", read_status, status);
                    LOG_MESSAGE(Logger::ERROR, "Unexpected response from AFL++ on forkserver setup.");
                    shotdown();
                    exit(1);
                }
            EASY_END_BLOCK

            while (true) {

                if(restart || _run_frist_loop){

                    //if restart is enabled and this is not the frist loop kill the old VP.
                    if(restart && !_run_frist_loop){
                        _vp_client->kill();
                        kill(_current_child, SIGKILL);
                        int status;
                        waitpid(-1, &status, WNOHANG);
                        LOG_MESSAGE(Logger::INFO, "VP instance %d killed, because restarting is enabled.", (int)_current_child);
                    }

                    _run_frist_loop = false;

                    //Start new VP instance.
                    _current_child = start_vp();
                    if(_current_child < 0){
                        LOG_MESSAGE(Logger::ERROR, "Creating child process!");
                        break;
                    }

                    // Waiting for VP to get ready
                    _vp_client->waitingForReady();
                    _vp_client->setup();
                }

                // TODO: dont know for what this is for !?
                int child_killed;
                if (read(_fksrv_ctl_fd, &child_killed, sizeof(child_killed)) != sizeof(child_killed)) {
                    LOG_MESSAGE(Logger::ERROR, "AFL parent exited before we could fork.");
                    shotdown();
                    exit(1);
                }

                LOG_MESSAGE(Logger::INFO, "Child Killed: %d", child_killed);

                if (child_killed > 0) {
                    
                    //TODO: recessary ?
                    /*
                    int status;
                    waitpid(-1, &status, WNOHANG); // Simplified waiting
                    status = swapEndian(status);
                    write(_fksrv_st_fd, &status, sizeof(status));
                    kill();
                    exit(1);
                    */
                }

                // Run with test case and writing coverage.
                /*
                EASY_BLOCK("Read test case");
                    std::string test_case = readFromTestcaseSharedMemory();
                    LOG_MESSAGE(Logger::INFO, "Test case loaded from shared memory: %s", test_case.c_str());
                EASY_END_BLOCK
                */

                EASY_BLOCK("Communicate child ID");
                    // Get pid and write it to AFL
                    if (write(_fksrv_st_fd, &_current_child, sizeof(_current_child)) != sizeof(_current_child)) {
                        LOG_MESSAGE(Logger::ERROR, "Failed to communicate with AFL.");
                        shotdown();
                        exit(1);
                    }
                EASY_END_BLOCK

                //vp_client->run(test_case, false);
                _vp_client->run_single("main", "exit", shm_input_id, 4);

                //EASY_BLOCK("Writing coverage"); 
                //    writeToSharedMemory(shm_cov_id, vp_client->bb_list, vp_client->bb_list_size);
                //EASY_END_BLOCK

                LOG_MESSAGE(Logger::INFO, "Writing code coverage");
                _vp_client->write_code_coverage(shm_cov_id, 0);

                LOG_MESSAGE(Logger::INFO, "Finished, sending status!");

                EASY_BLOCK("Communicate return value");  
                    int status = _vp_client->getRetValue();
                    if (write(_fksrv_st_fd, &status, sizeof(status)) != sizeof(status)) {
                        LOG_MESSAGE(Logger::ERROR, "Failed to send status to AFL.");
                        shotdown();
                        exit(1);
                    }
                EASY_END_BLOCK

                //Only profile a fixed number of runs
                #ifdef PROFILER_ENABLED
                    _profiler_count ++;
                    if(_profiler_count == PROFILING_COUNT){
                        LOG_MESSAGE(Logger::INFO, "Writing profiling file.");
                        profiler::dumpBlocksToFile("test_client.prof");
                    }
                #endif
            
            }

            _vp_client->kill();

            //TODO kill vp process!!!
        }

        void shotdown(){
            //TODO SIGTERM or SIGKILL ?
            _vp_client->kill();
            kill(_current_child, SIGKILL);
            int status;
            waitpid(-1, &status, WNOHANG);

            // Detaching shared memory, when attached.
            if (_shm_cov_data != nullptr && shmdt(_shm_cov_data) == -1) {
                LOG_MESSAGE(Logger::ERROR, "Failed to detach testcase shared memory: %s", strerror(errno));
            }

            if (_shm_input_data != nullptr && shmdt(_shm_cov_data) == -1) {
                LOG_MESSAGE(Logger::ERROR, "Failed to detach coverage shared memory: %s", strerror(errno));
            }
        }

        static void signal_handler(){
            signal_handler(15);
        }

        static void signal_handler(int sig) {
            // Handle SIGTERM or other signals as needed
            LOG_MESSAGE(Logger::ERROR, "Terminating on signal %d", sig);

            // killing current vp instance
            if(instance != nullptr){
                instance->shotdown();
            }

            exit(0);
        }

    private:
    
        VPClient* _vp_client;
        std::string _vp_executable;
        std::string _vp_launch_args;
        std::string _target_path;
        int _vp_loglevel = 0;
        std::string _vp_logging_path;
        int _profiler_count = 0;
        int _fksrv_st_fd = -1;
        int _fksrv_ctl_fd = -1;

        int _shm_cov_id = -1;
        char* _shm_cov_data;
        int _shm_input_id = -1;
        char* _shm_input_data;

        bool _run_frist_loop = true;

        pid_t _current_child = -1;

};

AFLForkserver* AFLForkserver::instance = nullptr;

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
                    Logger::init(logging_path, (Logger::LogLevel)logLevel);
                    LOG_MESSAGE(Logger::WARNING, "LOGGING ENABLED!");
                }
            }
            
            // Check required parameters
            if (argc != 6) {
                LOG_MESSAGE(Logger::ERROR, "Wrong parameters!");
                return 1;
            }

            const char* target_path = argv[1];
            LOG_MESSAGE(Logger::INFO, "Target program path: %s", target_path);

            int shm_input = 0;
            try{
                shm_input = std::stoi(argv[2]);
                LOG_MESSAGE(Logger::INFO, "Shared memory ID for test cases: %d", shm_input);
            }catch(std::exception &e){
                LOG_MESSAGE(Logger::INFO, "Argument test case shared memory is not a valid number!");
                return 1;
            }

            int shm_cov = 0;
            try{
                shm_cov = std::stoi(argv[3]);
                LOG_MESSAGE(Logger::INFO, "Shared memory ID for code coverage: %d", shm_cov);
            }catch(std::exception &e){
                LOG_MESSAGE(Logger::INFO, "Argument coverage shared memory is not a valid number!");
                return 1;
            }

            int ctl_fd = 0;
            try{
                ctl_fd = std::stoi(argv[4]);
                LOG_MESSAGE(Logger::INFO, "Forkserver control FD: %d", ctl_fd);
            }catch(std::exception &e){
                LOG_MESSAGE(Logger::INFO, "Argument for forkserver control FD is not a valid number!");
                return 1;
            }

            int st_fd = 0;
            try{
                st_fd = std::stoi(argv[5]);
                LOG_MESSAGE(Logger::INFO, "Forkserver status FD: %d", st_fd);
            }catch(std::exception &e){
                LOG_MESSAGE(Logger::INFO, "Argument for forkserver status FD is not a valid number!");
                return 1;
            }

            //Check settings vom environment variables
            const char* vp_executable = std::getenv("TC_VP_EXECUTABLE");
            if(vp_executable){
                LOG_MESSAGE(Logger::INFO, "VP executable (TC_VP_EXECUTABLE): %s", vp_executable);
            }else{
                LOG_MESSAGE(Logger::ERROR, "TC_VP_EXECUTABLE envirnoment variable not set!");
                return 1;
            }

            const char* vp_launch_args = std::getenv("TC_VP_LAUNCH_ARGS");
            if(vp_launch_args){
                LOG_MESSAGE(Logger::INFO, "VP launch args (TC_VP_LAUNCH_ARGS): %s", vp_launch_args);
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
                    LOG_MESSAGE(Logger::ERROR, "Could not parse value of TC_VP_LOGGING! Logging disabled.");
                }

                if(vp_loglevel > 0 && logging_path){
                    LOG_MESSAGE(Logger::INFO, "VP logging path (TC_VP_LOGGING_PATH): %s", vp_logging_path);
                }else{
                    LOG_MESSAGE(Logger::ERROR, "TC_VP_LOGGING_PATH envirnoment variable not set, but TC_VP_LOGGING enabled! Disabling VP logging.");
                    vp_loglevel = 0;
                    vp_logging_path = "";
                }

            }else{
                vp_logging_path = "";
            }

            const char* kill_old = std::getenv("TC_KILL_OLD");
            if(kill_old && strcmp(kill_old, "1") == 0){
                LOG_MESSAGE(Logger::INFO, "Killing old processes (TC_KILL_OLD) enabled.");
                
                std::string command = "killall "+std::string(vp_executable);
                int ret = system(command.c_str());
                if (ret == -1) LOG_MESSAGE(Logger::ERROR, "Error occoured while trying to killall %d. Continuing.", vp_executable);

                ret = system("killall --older-than 5s " OWN_NAME);
                if (ret == -1) LOG_MESSAGE(Logger::ERROR, "Error occoured while trying to killall %d. Continuing.", vp_executable);
            }

            //TODO check after killing! Or do it multiple times
            //TODO fedback to afl about errors

            const char* vp_restart_env = std::getenv("TC_VP_RESTART");
            bool vp_restart = false;
            if(vp_restart_env && strcmp(vp_restart_env, "1") == 0){
                LOG_MESSAGE(Logger::INFO, "VP restarting (TC_VP_RESTART) enabled.");
                vp_restart = true;
            }

            VPClient vPClient = VPClient();

            AFLForkserver aFLForkserver = AFLForkserver(&vPClient, vp_executable, vp_launch_args, target_path, vp_loglevel, vp_logging_path, st_fd, ctl_fd);

            // Set termination signal handler
            std::signal(SIGTERM, AFLForkserver::signal_handler);

        EASY_END_BLOCK

        
        EASY_BLOCK("Attaching shared memory segments");
            /*
            // Get the shared memory segment ID
            int shm_id_coverage = shmget((key_t)std::atoi(shm_cov_str), 0, 0666);  // 0 size, just attaching
            if (shm_id_coverage == -1) {
                LOG_MESSAGE(Logger::ERROR, "Failed to find shared memory segment: %s", strerror(errno));
                exit(1);
            }
            

           //LOG_MESSAGE(Logger::INFO, "Shared memory test: %s", getenv("__AFL_SHM_FUZZ_ID"));

            // Get the shared memory segment ID
            int shm_id_input = shmget(123457, 0, 0666);  // 0 size, just attaching
            if (shm_id_input == -1) {
                LOG_MESSAGE(Logger::ERROR, "Failed to find shared memory segment: %s", strerror(errno));
                exit(1);
            }

            */

        EASY_END_BLOCK;
        

        aFLForkserver.fksrv_start(shm_cov, shm_input, vp_restart);

    // Not AFL mode just does one simple run in avp64
    #else

        Logger::init("tc_out.txt", Logger::ALL);

        LOG_MESSAGE(Logger::INFO, "Run manual.");

        VPClient vPClient = VPClient();
        vPClient.waitingForReady();
        vPClient.setup();
        
        //vPClient.run("pasw", true);
        //vPClient.run("pass", false);
        //vPClient.kill();

        //TODO fix (with shared memory)
        vPClient.run_single("main", "exit", "pass");
        vPClient.write_code_coverage(12345);
        vPClient.kill();

        #ifdef PROFILER_ENABLED
            LOG_MESSAGE(Logger::INFO, "Writing profiling file.");
            profiler::dumpBlocksToFile("test_client.prof");
        #endif
    
    #endif

    return 0;
}
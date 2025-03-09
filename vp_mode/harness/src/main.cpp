#include "logger.h"
#include "afl_client.h"
#include "vp_client.h"

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
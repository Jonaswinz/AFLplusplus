/*
 * Copyright (C) 2025 ICE RWTH-Aachen
 *
 * This file is part of AFL++ VP-Mode.
 *
 * AFL++ VP-Mode is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * AFL++ VP-Mode is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with AFL++ VP-Mode. If not, see <https://www.gnu.org/licenses/>.
 */

#include "logger.h"
#include "afl_client.h"
#include "vp_client.h"
#include "settings.h"

//#undef AFL_MODE

int main(int argc, char* argv[]) {
    // Start profiler
    EASY_PROFILER_ENABLE;

    // AFL mode runs the forkserver with shared memory
    #ifdef AFL_MODE

        EASY_BLOCK("Setup");

            //Enable logging if env is set (H_LOGGING="1" and H_LOGGING_PATH to a path)
            const char* logging = std::getenv("H_LOGGING");
            const char* logging_path = std::getenv("H_LOGGING_PATH");
            int logLevel = 0;

            if(logging != nullptr){

                try{
                    logLevel = std::stoi(logging);
                }catch(std::exception &e){
                    return 1;
                }

                if(logLevel > 0 && logging_path){
                    logger::init(logging_path, (logger::log_level)logLevel);
                    LOG_MESSAGE(logger::WARNING, "--------------------------------------------");
                    LOG_MESSAGE(logger::WARNING, "\\ \\   / /  _ \\     |  \\/  | ___   __| | ___ ");
                    LOG_MESSAGE(logger::WARNING, " \\ \\ / /| |_| |____| |\\/| |/ _ \\ / _` |/ _ \\");
                    LOG_MESSAGE(logger::WARNING, "  \\ V / |  __/_____| |  | | |_| | |_| |  __/");
                    LOG_MESSAGE(logger::WARNING, "   \\_/  |_|        |_|  |_|\\___/ \\__,_|\\___|");
                    LOG_MESSAGE(logger::WARNING, "LOGGING ENABLED!");
                }
            }

            // Load ENV settings.
            settings::load();

            // Check parameters
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

            // Loading settings and parameters done.

            // Kill old
            if(settings::kill_old){
                LOG_MESSAGE(logger::INFO, "Killing old processes (H_KILL_OLD) enabled.");
                
                std::string command = "killall "+std::string(settings::vp_executable);
                int ret = system(command.c_str());
                if (ret == -1) LOG_MESSAGE(logger::ERROR, "Error occoured while trying to killall %d. Continuing.", settings::vp_executable);

                ret = system("killall --older-than 5s " OWN_NAME);
                if (ret == -1) LOG_MESSAGE(logger::ERROR, "Error occoured while trying to killall %d. Continuing.", settings::vp_executable);
            }

            afl_client m_afl_client = afl_client(st_fd, ctl_fd);

            // Set termination signal handler
            std::signal(SIGTERM, afl_client::signal_handler);
            std::signal(SIGINT, afl_client::signal_handler);
            std::signal(SIGKILL, afl_client::signal_handler);

            // Child process exit handler
            struct sigaction sa {};
            sa.sa_handler = afl_client::on_child_exit;
            sigemptyset(&sa.sa_mask);
            // This setting will terminate current pipe blocking reads and thus the request will be sent again, but to a new process.
            sa.sa_flags = 0;
            sigaction(SIGCHLD, &sa, nullptr);

        EASY_END_BLOCK
        
        LOG_MESSAGE(logger::WARNING, "Setup Done!");
        LOG_MESSAGE(logger::WARNING, "--------------------------------------------");
        
        m_afl_client.start(target_path, shm_cov, shm_input);

    // Not AFL mode just does one simple run in avp64
    #else

        logger::init("H_out.txt", logger::ALL);

        LOG_MESSAGE(logger::INFO, "Run manual.");

        vp_client m_vp_client = vp_client("/scratch/winzer/3/AFLplusplus/vp_mode/avp64/install/bin/avp64-runner", 1, "vp_out.txt", "-f", "/scratch/winzer/avp64-testing/benchmark/cortex-M0/arduino_json/arduino_json.cfg", 1073821732, 1073821732);
        m_vp_client.start_process();
        m_vp_client.waiting_for_ready();
        m_vp_client.setup();
        
        //vp_client.run("pasw", true);
        //vp_client.run("pass", false);
        //vp_client.kill();

        //TODO fix (with shared memory)
        //m_vp_client.run_single("main", "exit", "pass");
        //m_vp_client.write_code_coverage(12345);
        //m_vp_client.kill();

        #ifdef PROFILER_ENABLED
            LOG_MESSAGE(logger::INFO, "Writing profiling file.");
            profiler::dumpBlocksToFile("test_client.prof");
        #endif
    
    #endif

    return 0;
}
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

#ifndef DEFINES_H
#define DEFINES_H

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
#define MAX_VP_INSTANCES 40
//#define VP_GDB_SERVER
// End Settings

// Data to enable shared memory fuzzing for AFLplusplus
#define FS_OPT_ENABLED 0x80000001
#define FS_OPT_SHDMEM_FUZZ 0x01000000

// Logging macro
#define LOG_MESSAGE(type, format, ...) logger::log(type, format, ##__VA_ARGS__)

#endif
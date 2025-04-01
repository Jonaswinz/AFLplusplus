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

#ifndef LOGGER_H
#define LOGGER_H

#include "defines.h"

// Custom logger to log into a file
class logger {

    public:

        enum log_level{
            DISABLED, ALL, WARNINGS_AND_ERRORS
        };

        enum log_type{
            INFO, WARNING, ERROR
        };

        // Initialize the logger with a filename
        static void init(const std::string& filename, log_level set_log_level);

        // Log a message
        static void log(log_type msg_log_type, const std::string& format, ...);

        // Log a message
        static void log(log_type msg_log_type, const std::string& format, va_list args);

        static void log_error(const char* fmt, ...);

        static void log_info(const char* fmt, ...);

        // Clean up the logger
        static void close();

    private:
        // Mutex to synchonize logging when multiple threads are logging to the same file.
        static std::mutex logger_mutex;

        static std::ofstream file_stream;
        static log_level selected_log_level;
};


#endif
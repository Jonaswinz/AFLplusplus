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
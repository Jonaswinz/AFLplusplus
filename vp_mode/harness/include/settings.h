#ifndef SETTINGS_H
#define SETTINGS_H

#include "defines.h"
#include "logger.h"
#include "vp_client.h"

class settings{

    public:

        static char* vp_executable;
        static char* vp_launch_args;
        static int vp_log_level;
        static char* vp_logging_path;

        static bool kill_old;

        static int mode;
        static int vp_instances;
        static int vp_instance_restarter;

        static char* run_start_symbol;
        static char* run_end_symbol;
        static char* run_return_register;
        static char* error_symbol;
        static int run_mmio_data_address;
        static int run_mmio_data_length;
        static std::vector<vp_client::fixed_read> fixed_reads;
        static std::vector<vp_client::interrupt_trigger> interrupt_triggers;

        static void load();

    private:

        static void parse_fixed_reads(const char* env);
        static void parse_interrupt_triggers(const char* env);
};



#endif
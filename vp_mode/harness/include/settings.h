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
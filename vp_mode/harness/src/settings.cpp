#include "settings.h"

void settings::load(){

    // VP
    vp_executable = std::getenv("H_VP_EXECUTABLE");
    if(vp_executable){
        LOG_MESSAGE(logger::INFO, "VP executable (H_VP_EXECUTABLE): %s", vp_executable);
    }else{
        LOG_MESSAGE(logger::ERROR, "H_VP_EXECUTABLE envirnoment variable not set!");
        exit(1);
    }

    vp_launch_args = std::getenv("H_VP_LAUNCH_ARGS");
    if(vp_launch_args){
        LOG_MESSAGE(logger::INFO, "VP launch args (H_VP_LAUNCH_ARGS): %s", vp_launch_args);
    }else{
        vp_launch_args = strdup("");
    }

    const char* vp_log_level_temp = std::getenv("H_VP_LOGGING");
    vp_logging_path = std::getenv("H_VP_LOGGING_PATH");

    if(vp_log_level_temp){

        try{
            vp_log_level = std::stoi(vp_log_level_temp);
        }catch(std::exception &e){
            LOG_MESSAGE(logger::ERROR, "Could not parse value of H_VP_LOGGING! Logging disabled.");
        }

        if(vp_log_level > 0 && vp_logging_path){
            LOG_MESSAGE(logger::INFO, "VP logging path (H_VP_LOGGING_PATH): %s", vp_logging_path);
        }else{
            LOG_MESSAGE(logger::ERROR, "H_VP_LOGGING_PATH envirnoment variable not set, but H_VP_LOGGING enabled! Disabling VP logging.");
            vp_log_level = 0;
            vp_logging_path = strdup("");
        }

    }else{
        vp_logging_path = strdup("");
    }

    // Kill old
    const char* kill_old_temp = std::getenv("H_KILL_OLD");
    if(kill_old_temp && strcmp(kill_old_temp, "1") == 0){
        kill_old = true;
    }

    // Harness mode
    const char* mode_temp = std::getenv("H_MODE");
    if(mode_temp){
        try{
            mode = std::stoi(mode_temp);
            LOG_MESSAGE(logger::INFO, "Test client mode (H_MODE) set to: %d", mode);
        }catch(std::exception &e){
            LOG_MESSAGE(logger::ERROR, "Could not parse value of H_MODE!");
            exit(1);
        }
    }else{
        LOG_MESSAGE(logger::ERROR, "H_MODE envirnoment variable not set, but is required!");
        exit(1);
    }

    const char* vp_instances_temp = std::getenv("H_VP_INSTANCES");
    if(vp_instances_temp){
        try{
            vp_instances = std::stoi(vp_instances_temp);
            if(vp_instances < 1){
                vp_instances = 1;
            }
            LOG_MESSAGE(logger::INFO, "Number of VP instances (H_VP_INSTANCES) set to: %d", vp_instances);
        }catch(std::exception &e){
            LOG_MESSAGE(logger::ERROR, "Could not parse value of H_VP_INSTANCES! Set to defaut: 1.");
            vp_instances = 1;
        }
    }

    const char* vp_instance_restarter_temp = std::getenv("H_VP_INSTANCE_RESTARTER");
    if(vp_instance_restarter_temp){
        try{
            vp_instance_restarter = std::stoi(vp_instance_restarter_temp);
            if(vp_instance_restarter < 1){
                vp_instance_restarter = 1;
            }
            LOG_MESSAGE(logger::INFO, "Number of VP instance restarter (H_VP_INSTANCE_RESTARTER) set to: %d", vp_instance_restarter);
        }catch(std::exception &e){
            LOG_MESSAGE(logger::ERROR, "Could not parse value of H_VP_INSTANCE_RESTARTER! Set to defaut: 1.");
            vp_instance_restarter = 1;
        }
    }

    // Do run
    run_start_symbol = std::getenv("H_START_SYMBOL");
    if(run_start_symbol){
        LOG_MESSAGE(logger::INFO, "Start symbol (H_START_SYMBOL): %s", run_start_symbol);
    }else{
        LOG_MESSAGE(logger::ERROR, "H_START_SYMBOL envirnoment variable not set!");
        exit(1);
    }

    run_end_symbol = std::getenv("H_END_SYMBOL");
    if(run_end_symbol){
        LOG_MESSAGE(logger::INFO, "End symbol (H_END_SYMBOL): %s", run_end_symbol);
    }else{
        LOG_MESSAGE(logger::ERROR, "H_END_SYMBOL envirnoment variable not set!");
        exit(1);
    }

    run_return_register = std::getenv("H_RETURN_REGISTER");
    if(run_return_register){
        LOG_MESSAGE(logger::INFO, "Return register (H_RETURN_REGISTER): %s", run_return_register);
    }else{
        LOG_MESSAGE(logger::ERROR, "H_RETURN_REGISTER envirnoment variable not set!");
        exit(1);
    }

    error_symbol = std::getenv("H_ERROR_SYMBOL");
    if(error_symbol){
        LOG_MESSAGE(logger::INFO, "Error symbol (H_ERROR_SYMBOL): %s", error_symbol);
    }else{
        error_symbol = strdup("");
    }

    const char* mmio_data_address_temp = std::getenv("H_MMIO_DATA_ADDRESS");
    if(mmio_data_address_temp){
        try{
            run_mmio_data_address = std::stoul(mmio_data_address_temp, nullptr, 16);
            LOG_MESSAGE(logger::INFO, "MMIO data address (H_MMIO_DATA_ADDRESS) set to: %d", run_mmio_data_address);
        }catch(std::exception &e){
            LOG_MESSAGE(logger::ERROR, "Could not parse value of H_MMIO_DATA_ADDRESS!");
            exit(1);
        }
    }else{
        LOG_MESSAGE(logger::ERROR, "H_MMIO_DATA_ADDRESS envirnoment variable not set, but is required!");
        exit(1);
    }

    const char* mmio_data_length_temp = std::getenv("H_MMIO_DATA_LENGTH");
    if(mmio_data_length_temp){
        try{
            run_mmio_data_length = std::stoi(mmio_data_length_temp);
            LOG_MESSAGE(logger::INFO, "MMIO data length (H_MMIO_DATA_LENGTH) set to: %d", run_mmio_data_length);
        }catch(std::exception &e){
            LOG_MESSAGE(logger::ERROR, "Could not parse value of H_MMIO_DATA_LENGTH!");
            exit(1);
        }
    }else{
        LOG_MESSAGE(logger::ERROR, "H_MMIO_DATA_LENGTH envirnoment variable not set, but is required!");
        exit(1);
    }

    const char* fixed_reads_temp = std::getenv("H_FIXED_READS");
    if (fixed_reads_temp) {
        parse_fixed_reads(fixed_reads_temp);
    }

    const char* interrupt_triggers_temp = std::getenv("H_INTERRUPT_TRIGGERS");
    if (interrupt_triggers_temp) {
        parse_interrupt_triggers(interrupt_triggers_temp);
    }
}

void settings::parse_fixed_reads(const char* env){
    std::string input(env);
    std::stringstream ss(input);
    std::string pair;

    while (std::getline(ss, pair, ';')) {
        std::stringstream pair_stream(pair);
        std::string addr_str, data_str;

        if (!std::getline(pair_stream, addr_str, ',') || !std::getline(pair_stream, data_str, ',')) {
            LOG_MESSAGE(logger::ERROR, "Invalid format in H_FIXED_READS!");
            continue;
        }

        try {
            uint64_t addr = std::stoull(addr_str, nullptr, 16);
            uint64_t data_val = std::stoull(data_str, nullptr, 16);

            vp_client::fixed_read fr;
            fr.address = addr;
            fr.data = static_cast<char>(data_val);
            fixed_reads.push_back(fr);
            LOG_MESSAGE(logger::INFO, "Added fixed read: 0x%08lx, %d", fr.address, fr.data);
        } catch (const std::exception& e) {
            LOG_MESSAGE(logger::ERROR, "Failed to parse H_FIXED_READS!");
        }
    }
}

void settings::parse_interrupt_triggers(const char* env){
    std::string input(env);
    std::stringstream ss(input);
    std::string pair;

    while (std::getline(ss, pair, ';')) {
        std::stringstream pair_stream(pair);
        std::string address_str, trigger_str;

        if (!std::getline(pair_stream, address_str, ',') || !std::getline(pair_stream, trigger_str, ',')) {
            LOG_MESSAGE(logger::ERROR, "Invalid format in H_INTERRUPT_TRIGGERS!");
            continue;
        }

        try {
            uint64_t interrupt_address = std::stoull(address_str, nullptr, 16);
            uint64_t trigger_address = std::stoull(trigger_str, nullptr, 16);

            vp_client::interrupt_trigger trigger;
            trigger.interrupt_address = interrupt_address;
            trigger.trigger_address = trigger_address;
            interrupt_triggers.push_back(trigger);
            LOG_MESSAGE(logger::INFO, "Added interrupt trigger: 0x%08lx (interrupt) at 0x%08lx (trigger)", trigger.interrupt_address, trigger.trigger_address);
        } catch (const std::exception& e) {
            LOG_MESSAGE(logger::ERROR, "Failed to parse H_INTERRUPT_TRIGGERS!");
        }
    }
}

// Default
char* settings::vp_executable = nullptr;
char* settings::vp_launch_args = nullptr;
int settings::vp_log_level = 0;
char* settings::vp_logging_path = nullptr;
bool settings::kill_old = false;
int settings::mode = 0;
int settings::vp_instances = 1;
int settings::vp_instance_restarter = 1;
char* settings::run_start_symbol = nullptr;
char* settings::run_end_symbol = nullptr;
char* settings::run_return_register = nullptr;
char* settings::error_symbol = nullptr;
int settings::run_mmio_data_address = 0;
int settings::run_mmio_data_length = 1;
std::vector<vp_client::fixed_read> settings::fixed_reads;
std::vector<vp_client::interrupt_trigger> settings::interrupt_triggers;
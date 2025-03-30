## Required
# Path to harness
export H_PATH="$(pwd)/vp_mode/harness/build/harness"
# Path vp
export H_VP_EXECUTABLE="$(pwd)/vp_mode/VPs/avp64/install/bin/avp64-runner"
# Launch arguments of vp
export H_VP_LAUNCH_ARGS="-f"
# Add required libraries
export LD_LIBRARY_PATH="$LD_LIBRARY_PATH:$(pwd)/vp_mode/VPs/avp64/install/lib/:$(pwd)/vp_mode/VPs/avp64/install/lib64/"

## Optional
# 0: No logging (default), 1: Everything, 2: Errors only
export H_LOGGING="0"
export H_LOGGING_PATH="h_out.txt"
# 0: No logging (default), 1: Everything, 2: Errors only
export H_VP_LOGGING="0"
export H_VP_LOGGING_PATH="vp_out.txt"
# 0: Not killing (default), 1: Killing old instances
export H_KILL_OLD="1"

# Mode
# 0: Restart after each run, 1: Persistent mode
export H_MODE="1"
# Number of the VP process instances used. Required for H_MODE=0.
export H_VP_INSTANCES="1"
# Fuzzing Settings
export H_START_SYMBOL="main"
export H_END_SYMBOL="exit"
export H_RETURN_REGISTER="X0"
export H_MMIO_DATA_ADDRESS="0x10009518"
export H_MMIO_DATA_LENGTH="1"

## Additional for AFLplusplus
export AFL_I_DONT_CARE_ABOUT_MISSING_CRASHES="1"
export AFL_SKIP_CPUFREQ="1"

#Additional for setup
export LD_LIBRARY_PATH="$LD_LIBRARY_PATH:/net/sw/gcc/gcc-11.4.1/lib64"
export AFL_SKIP_CPUFREQ=1
export AFL_I_DONT_CARE_ABOUT_MISSING_CRASHES=1

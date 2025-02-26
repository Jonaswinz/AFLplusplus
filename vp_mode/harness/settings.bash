## Required
# Path to harness
export TC_PATH="$(pwd)/vp_mode/harness/build/test_client"
# Path vp
export TC_VP_EXECUTABLE="$(pwd)/vp_mode/avp64/install/bin/avp64-runner"
# Launch arguments of vp
export TC_VP_LAUNCH_ARGS="--enable-test-receiver --test-receiver-interface 1 --test-receiver-pipe-request 10 --test-receiver-pipe-response 11 -f"
# Add required libraries
export LD_LIBRARY_PATH="$LD_LIBRARY_PATH:$(pwd)/vp_mode/avp64/install/lib/:$(pwd)/vp_mode/avp64/install/lib64/"

## Optional
# 0: No logging (default), 1: Everything, 2: Errors only
export TC_LOGGING="2"
export TC_LOGGING_PATH="tc_out.txt"
# 0: No logging (default), 1: Everything, 2: Errors only
export TC_VP_LOGGING="2"
export TC_VP_LOGGING_PATH="vp_out.txt"
# 0: Not killing (default), 1: Killing old instances
export TC_KILL_OLD="1"
# 0: Not restarting (default), 1: Restarting
export TC_VP_RESTART="0"

# Mode
# 0: Restart after each run, 1: Persistent mode
export TC_MODE="0"
# Number of the VP process instances used. Required for TC_MODE=0.
export TC_VP_INSTANCES="0"
# Fuzzing Settings
export TC_START_SYMBOL="main"
export TC_END_SYMBOL="exit"
export TC_RETURN_REGISTER="x0"
export TC_MMIO_DATA_ADDRESS="0x10009518"

## Additional for AFLplusplus
export AFL_I_DONT_CARE_ABOUT_MISSING_CRASHES="1"
export AFL_SKIP_CPUFREQ="1"

#Additional for setup
export LD_LIBRARY_PATH="$LD_LIBRARY_PATH:/net/sw/gcc/gcc-11.4.1/lib64"
export AFL_SKIP_CPUFREQ=1
export AFL_I_DONT_CARE_ABOUT_MISSING_CRASHES=1

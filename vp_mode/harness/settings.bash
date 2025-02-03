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
# 0: Run between symbols (default), 1: Snapshot to symbol
export TC_MODE="0"
# Start symbol for on run. Required for TC_MODE=0,1
export TC_START_SYMBOL="main"
# End symbol for a run. Required for TC_MODE=0
export TC_END_SYMBOL="exit"
# Path to the snapshot files, that should be loaded. Required for TC_MODE=1
export TC_SNAPSHOT_PATH="/target/snapshot"

## Additional for AFLplusplus
export AFL_I_DONT_CARE_ABOUT_MISSING_CRASHES="1"
export AFL_SKIP_CPUFREQ="1"

#Additional for setup
export LD_LIBRARY_PATH="$LD_LIBRARY_PATH:/net/sw/gcc/gcc-11.4.1/lib64"


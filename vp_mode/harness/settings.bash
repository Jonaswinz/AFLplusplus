#AFL ENV
export AFL_I_DONT_CARE_ABOUT_MISSING_CRASHES="1"
export LD_LIBRARY_PATH="$LD_LIBRARY_PATH:/net/sw/gcc/gcc-11.4.1/lib64:/test_client:/AFLplusplus/vp_mode/avp64/build/ocx-qemu-arm"
export AFL_SKIP_CPUFREQ="1"
export TC_PATH="/AFLplusplus/vp_mode/harness/build/test_client"

#Test Client ENV
export TC_VP_EXECUTABLE="avp64-runner"
export TC_VP_LAUNCH_ARGS="-f"
export TC_LOGGING="2"
export TC_LOGGING_PATH="tc_out.txt"
export TC_VP_LOGGING="1"
export TC_VP_LOGGING_PATH="vp_out.txt"
export TC_KILL_OLD="1"
export TC_VP_RESTART="0"
#Mode
#TC_MODE (0: Run between symbols, 1: Snapshot to symbol)
export TC_MODE="0"
#TC_START_SYMBOL Required for TC_MODE=0,1
export TC_START_SYMBOL="main"
#TC_START_SYMBOL Required for TC_MODE=0
export TC_END_SYMBOL="exit"
#TC_SNAPSHOT_PATH Required for TC_MODE=1
export TC_SNAPSHOT_PATH="/target/snapshot"

#Additional ENV
export LD_LIBRARY_PATH="$LD_LIBRARY_PATH:/net/sw/gcc/gcc-11.4.1/lib64"


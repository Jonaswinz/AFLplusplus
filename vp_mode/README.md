# Binary fuzzing with a virtual platform (avp64)

## 1) Introduction
TODO

## 2) Build VP mode
When running `make distrib` or `make binary-only` the vp-mode will be built unless the `NO_VPMODE` env is set. If avp64 and the harness should not be build automatically with AFLplusplus please set `NO_VPMODE`. You can still run the vp-mode (`-v`), by specifying an alternative location for `test_client` and `avp64-runner` (see section 3).

## 3) How to use VP mode
There a some settings that need to be set, before running the harness (test_client). Because of $(pwd), these commands need to be executed from the AFLplusplus directory.

### Required settings
```
## General Settings
# Path to harness
export TC_PATH="$(pwd)/vp_mode/harness/build/test_client"
# Path vp
export TC_VP_EXECUTABLE="$(pwd)/vp_mode/avp64/install/bin/avp64-runner"
# Launch arguments of vp
export TC_VP_LAUNCH_ARGS="-f"
# Add required libraries
export LD_LIBRARY_PATH="$LD_LIBRARY_PATH:$(pwd)/vp_mode/avp64/install/lib/:$(pwd)/vp_mode/avp64/install/lib64/"

## Settings of the execution / mode
# 0: Restart after each run, 1: Persistent mode
export TC_MODE="0"
# Fuzzing Settings
# Start symbol of the part where the fuzzing takes place
export TC_START_SYMBOL="main"
# End symbol of the part where the fuzzing takes place
export TC_END_SYMBOL="exit"
# Register where the return code should be read from (when the end symbol is hit)
export TC_RETURN_REGISTER="x0"
# Address of the fuzzing / where the data is injected.
export TC_MMIO_DATA_ADDRESS="0x10009518"
```

### Optional settings
```
# 0: No logging (default), 1: Everything, 2: Errors only
export TC_LOGGING="2"
export TC_LOGGING_PATH="tc_out.txt"
# 0: No logging (default), 1: Everything, 2: Errors only
export TC_VP_LOGGING="2"
export TC_VP_LOGGING_PATH="vp_out.txt"
# 0: Not killing (default), 1: Killing old instances
export TC_KILL_OLD="1"
# Number of the VP process instances used. Required for TC_MODE=0.
export TC_VP_INSTANCES="0"
```

Optionally the `settings.bash` inside the harness directory can be modified and executed. After this the vp-mode can be started with the `-v` option, for example like this:

```
afl-fuzz -i <seeds folder> -o <out folder> -m none -v -- <path to target.cfg>
```

A Visual Studio Code launch.json configuration (paste inside "configurations": []) can look like this. Note: Not all settings for the harness must be set, their default value will then be used.

```
{
    "name": "improved this",
    "type": "cppdbg",
    "request": "launch",
    "program": "${workspaceFolder}/afl-fuzz",
    "args": [
    "-i",
    "/fuzzing/arch_pro_seeds/",
    "-o",
    "/fuzzing/build/out",
    "-m",
    "none",
    "-v",
    "--",
    "/fuzzing/arch_pro_minimal_loop/arch_pro.cfg"
    ],
    "stopAtEntry": false,
    "cwd": "${workspaceFolder}",
    "environment": [
        {
          "name": "AFL_I_DONT_CARE_ABOUT_MISSING_CRASHES",
          "value": "1"
        },
        {
          "name": "LD_LIBRARY_PATH",
          "value": "$LD_LIBRARY_PATH:/net/sw/gcc/gcc-11.4.1/lib64:/scratch/winzer/jw/avp64-mq/build/debug/build/ocx-qemu-arm"
        },
        {
          "name": "AFL_SKIP_CPUFREQ",
          "value": "1"
        },
        {
          "name": "TC_PATH",
          "value": "/scratch/winzer/test_client/cpp/final/build/test_client"
        },
        {
          "name": "TC_VP_EXECUTABLE",
          "value": "/scratch/winzer/jw/avp64-mq/build/debug/build/avp64-runner"
        },
        {
          "name": "TC_VP_LAUNCH_ARGS",
          "value": "-f"
        },
        {
          "name": "TC_LOGGING",
          "value": "1"
        },
        {
          "name": "TC_LOGGING_PATH",
          "value": "tc_out.txt"
        },
        {
          "name": "TC_VP_LOGGING",
          "value": "1"
        },
        {
          "name": "TC_VP_LOGGING_PATH",
          "value": "vp_out.txt"
        },
        {
          "name": "TC_KILL_OLD",
          "value": "1"
        },
        {
          "name": "TC_MODE",
          "value": "0"
        },
        {
          "name": "TC_VP_INSTANCES",
          "value": "20"
        },
        {
          "name": "TC_START_SYMBOL",
          "value": "main"
        },
        {
          "name": "TC_END_SYMBOL",
          "value": "exit"
        },
        {
          "name": "TC_RETURN_REGISTER",
          "value": "x0"
        },
        {
          "name": "TC_MMIO_DATA_ADDRESS",
          "value": "0x10009518"
        }
    ]
}
```

## ...
TODO
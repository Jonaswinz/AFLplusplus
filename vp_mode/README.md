# Binary fuzzing with a virtual platform (avp64)

## 1) Introduction
TODO

## 2) Build VP mode
When running `make distrib` or `make binary-only` the vp-mode will be built unless the `NO_VPMODE` env is set. If avp64 and the harness should not be build automatically with AFLplusplus please set `NO_VPMODE`. You can still run the vp-mode (`-v`), by specifying an alternative location for `test_client` and `avp64-runner` (see section 3).

## 3) How to use VP mode
There a some settings that need to be set, before running the harness (test_client). Because of $(pwd), these commands need to be executed from the AFLplusplus directory.

### Required settings
```
# Path to harness
export TC_PATH="$(pwd)/vp_mode/harness/build/test_client"
# Path vp
export TC_VP_EXECUTABLE="$(pwd)/vp_mode/avp64/install/bin/avp64-runner"
# Launch arguments of vp
export TC_VP_LAUNCH_ARGS="--enable-test-receiver --test-receiver-interface 1 --test-receiver-pipe-request 10 --test-receiver-pipe-response 11 -f"
# Add required libraries
export LD_LIBRARY_PATH="$LD_LIBRARY_PATH:$(pwd)/vp_mode/avp64/install/lib/:$(pwd)/vp_mode/avp64/install/lib64/"
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
        "name": "LD_LIBRARY_PATH",
        "value": "$LD_LIBRARY_PATH:${workspaceFolder}/vp_mode/avp64/install/lib:${workspaceFolder}/vp_mode/avp64/install/lib64:/net/sw/gcc/gcc-11.4.1/lib64"
    },
    {
        "name": "AFL_SKIP_CPUFREQ",
        "value": "1"
    },
    {
        "name": "TC_PATH",
        "value": "vp_mode/harness/build/test_client"
    },
    {
        "name": "TC_VP_EXECUTABLE",
        "value": "vp_mode/avp64/install/bin/avp64-runner"
    },
    {
        "name": "TC_VP_LAUNCH_ARGS",
        "value": "--enable-test-receiver --test-receiver-interface 1 --test-receiver-pipe-request 10 --test-receiver-pipe-response 11 -f"
    },
    {
        "name": "TC_LOGGING",
        "value": "2"
    },
    {
        "name": "TC_LOGGING_PATH",
        "value": "tc_out.txt"
    },
    {
        "name": "TC_VP_LOGGING",
        "value": "2"
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
        "name": "TC_VP_RESTART",
        "value": "0"
    }
    ]
}
```

## ...
TODO
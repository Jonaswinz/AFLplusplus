# Binary Fuzzing with a Virtual Platform (VP Mode)

## 1) Introduction
*TODO*

---

## 2) Build VP Mode

**_Recommended GCC version: 11.4.0_**

1. Clone this fork of the AFL++ repository.

2. Modify the `VP_VERSION` file in the `vp_mode` folder to specify which VPs should be cloned and built.  
   Some are already listed. Comment out lines with `#` to disable specific entries.

3.  
   **a)** VP mode (`-v`) is another `binary-only` mode of AFL++ and integrated into its build process.   Running `make distrib` or `make binary-only` will also build the vp-mode **unless** the `NO_VPMODE` environment variable is set.

   **b)** To build only the vp-mode (recommended), run:

   ```bash
   cd vp_mode
   ./build_vp_support.sh
   ```

4. *(Optional)* After modifying any VP, rerun `./build_vp_support.sh` or run `make` in `vp_mode`,  
   or `make install` in the specific VP directory inside `VPs`.

---

## 3) How to Use VP Mode

To enable VP mode, use the `-v` parameter with `afl-fuzz`.

**Before fuzzing**, several environment variables must be set:

| ENV Variable               | Required        | Description |
|---------------------------|-----------------|-------------|
| `H_PATH`                  | Required     | Path to the harness executable. Usually `vp_mode/harness/build/harness`. |
| `H_VP_EXECUTABLE`         | Required     | Path to the VP executable. Usually `vp_mode/VPs/<VP>/install/bin/<VP>`. |
| `H_VP_LAUNCH_ARGS`        | Optional        | Additional arguments passed to the VP. |
| `LD_LIBRARY_PATH`         | Required     | Required library paths for the VP. |
| `H_LOGGING`               | Optional        | Harness logging level: `0 = off` (default), `1 = verbose`, `2 = errors only`. |
| `H_LOGGING_PATH`          | Optional        | Path to harness log file. |
| `H_VP_LOGGING`            | Optional        | VP logging level: `0 = off` (default), `1 = verbose`, `2 = errors only`. |
| `H_VP_LOGGING_PATH`       | Optional        | Path to VP log file. |
| `H_KILL_OLD`              | Optional        | Kill leftover VP instances on startup: `0 = no` (default), `1 = yes`. |
| `H_MODE`                  | Required     | Harness mode: `0 = Restart after each run`, `1 = Persistent mode`. |
| `H_VP_INSTANCES`          | Optional        | Number of parallel VP instances (required for `H_MODE=0`). |
| `H_VP_INSTANCE_RESTARTER` | Optional        | Number of threads restarting VP instances (used with `H_MODE=0`). |
| `H_START_SYMBOL`          | Required     | Symbol marking the start of the fuzzed region. Can be `""`. |
| `H_END_SYMBOL`            | Required     | Symbol marking the end of the fuzzed region. Value in `H_RETURN_REGISTER` is read here. |
| `H_RETURN_REGISTER`       | Required     | Register to read at `H_END_SYMBOL` (e.g., `r0`). |
| `H_ERROR_SYMBOL`          | Optional        | If reached, execution aborts with return code 0. Useful for exception handlers. |
| `H_MMIO_DATA_ADDRESS`     | Required     | MMIO read address to intercept and fill with test data. |
| `H_MMIO_DATA_LENGTH`      | Required     | Number of bytes to write to the MMIO request at `H_MMIO_DATA_ADDRESS` in every interception. |
| `H_FIXED_READS`           | Optional        | Fixed MMIO responses: `<hex_addr>,<hex_byte>;...` (e.g., `0x1000,0x01` to respond with 1 when `0x1000` is read). |
| `H_INTERRUPT_TRIGGERS`    | Optional        | Trigger interrupts: `<isr_addr>,<trigger_addr>;...` (e.g., `0x2000,0x1000` to trigger interrupt `0x2000` when instruction at `0x1000` is encountered). |
| `AFL_SKIP_CPUFREQ`        | Required     | AFL++ setting to avoid CPU frequency scaling checks. |
| `AFL_I_DONT_CARE_ABOUT_MISSING_CRASHES` | Required | AFL++ setting to ignore crash detection. |


> ℹ️ See `example_target/Settings.bash` for a complete environment setup example.

---

### Fuzzing Command
To start the vp-mode AFL++ can be started like this:

```bash
afl-fuzz -i <seeds_folder> -o <out_folder> -m none -v -- <path_to_target.cfg>
```
The `<path_to_target.cfg>` config file is passed to the VP, which contains the information what target software should be started.

---

## 4) Example Target

The `example_target` folder provides a complete fuzzing setup using AVP64. You can edit `Settings.bash` to modify harness behavior. Start the example with (from the repositories root):

```bash
./vp_mode/example_target/run.bash
```

This script:
- Sets up the environment
- Prepares the output directory
- Runs AFL++

---

## 5) Troubleshooting

- Enable harness and VP logging if fuzzing fails.
- If start/end symbols aren't found, ensure they're not optimized away or renamed by C++ — use `extern "C"`.
- For interrupts, remember: the interrupt triggers **before** the instruction is executed. After returning, the original instruction is executed.
- In persistent mode, make sure the loop boundaries are safe to avoid stack overflows.


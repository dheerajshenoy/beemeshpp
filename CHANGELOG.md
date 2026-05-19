# CHANGELOG

## 0.1.1 [WIP]

- Add `--version` flag to print version and exit
- Add `--help` flag to print usage information and exit
- Show elapsed time for each job in the monitor dashboard
- Add `launch` subcommand to submit jobs to the hive from any machine; supports shell commands and local files:
  ```bash
  beemesh launch --payload "echo hello"
  beemesh launch --payload ./my_script.sh
  beemesh launch --payload /path/to/script.py
  ```
- Add `JOB_SUBMISSION` and `JOB_RESULT` message types to the protocol
- Hive dispatches queued jobs to idle bees using round-robin assignment
- Bees execute job payloads on their local device in a worker thread, keeping the event loop responsive during execution; file payloads are written to a temp file, executed, then deleted
- Result is sent back to the hive and printed when the job completes
- Hive re-queues in-flight jobs when a bee disconnects
- Auth token validation on bee registration — connections with a bad token are rejected
- Full async connection handling in the hive — initial handshake no longer blocks the io_context
- Bees track idle/busy state; the hive only dispatches to idle bees
- Add `monitor` subcommand with a live ftxui TUI showing connected bees, their status, and job stats (pending/running/completed); hive broadcasts a status snapshot on every state change
- Monitor shows the TUI immediately on TCP connect even with 0 bees
- Monitor shows a clear error message if the hive is unreachable
- Bees report real device info on registration (CPU model/cores/threads/arch, GPU vendor/name/VRAM, RAM, OS) using OS-level APIs (`/proc/cpuinfo`, `/sys/class/drm/`, `sysctlbyname` on macOS)
- Monitor dashboard shows hostname and OS columns for each connected bee
- Add optional benchmarking on bee registration (`beemesh hive --benchmark`); hive sends a `BENCHMARK_REQUEST` to each new bee, bee runs native C++ CPU (double-precision GFLOPS) and memory bandwidth (GB/s) benchmarks, results stored per bee and shown in the monitor detail panel; bees are held out of the job pool until benchmarking completes
- Job dispatch selects the best eligible bee by benchmark score (`cpu_gflops + mem_bandwidth_gbps`) rather than first-available; falls back to first-available when benchmarking is disabled
- Add `#BEEMESH --min-gflops` and `#BEEMESH --min-mem-bw` directives to filter bees by benchmark threshold; jobs stay queued until a bee meeting all requirements is available
- Monitor table is interactive: navigate rows with `j`/`k` or arrow keys; press Enter to open a detail panel showing job output, exit code, and scrollable stdout/stderr
- Track job exit codes end-to-end — bee extracts the real exit code from `pclose`, sends it with the result, hive stores and broadcasts it, monitor shows it per-bee in the table and detail panel (green for 0, red for non-zero)
- MSVC compatibility: `_popen`/`_pclose` on Windows, `<windows.h>`/`<dxgi.h>` included for GPU/RAM/hostname APIs, `filesystem::permissions` guarded on non-Windows, MSVC compile flags and `dxgi` linkage added to CMake
- Add `#BEEMESH` script directives (inspired by `#SBATCH`) for embedding job requirements directly in submitted scripts:
  ```bash
  #BEEMESH --name my-analysis
  #BEEMESH --cpus 4
  #BEEMESH --mem 16G
  #BEEMESH --gpu
  #BEEMESH --target my-hostname
  ```
- Hive matches job requirements against bee capabilities (CPU cores, RAM, GPU) collected at registration time; a job stays queued until a satisfying bee is available
- Monitor table shows job ID and name together (e.g. `#3 my-analysis`); detail panel also includes the name
- Stream job output in real time: bees send `JOB_OUTPUT` chunks to the hive as the job runs (flushed every 200 ms or every 4096 bytes); hive prints and accumulates chunks; monitor detail panel shows live stdout/stderr without waiting for job completion

## 0.1.0

- Add command line arguments
- Add asio library for network communication

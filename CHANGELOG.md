# CHANGELOG

## 0.1.1 [WIP]

- Add `launch` subcommand to submit jobs to the hive from any machine
- Add `JOB_SUBMISSION` and `JOB_RESULT` message types to the protocol
- Hive dispatches queued jobs to idle bees using round-robin assignment
- Bees execute job payloads via `popen` on their local device in a worker thread, keeping the event loop responsive during execution
- Result is sent back to the hive and printed when the job completes
- Hive re-queues in-flight jobs when a bee disconnects
- Auth token validation on bee registration — connections with a bad token are rejected
- Full async connection handling in the hive — initial handshake no longer blocks the io_context
- Bees track idle/busy state; the hive only dispatches to idle bees

## 0.1.0

- Add command line arguments
- Add asio library for network communication

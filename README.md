# beemeshpp
BeeMesh implementation in C++

A distributed task execution system where a central **hive** receives jobs and dispatches them to connected **bee** nodes across the network.

## Building and Running

```bash
cmake -B build
cmake --build build
cd build
./beemesh
```

## Usage

### Start the hive

The hive accepts connections from bees and launchers. Run this on the machine that will coordinate work:

```bash
beemesh hive --host 0.0.0.0 --port 9000 --auth-token mytoken
```

### Register a bee

Bees connect to the hive and wait for jobs to execute. Run this on each worker machine:

```bash
beemesh bee --host <hive-ip> --port 9000 --auth-token mytoken
```

### Submit a job

The `launch` command submits work to the hive from any machine. Jobs are dispatched to an idle bee and executed there.

**Shell command:**
```bash
beemesh launch --host <hive-ip> --port 9000 --payload "uname -a"
```

**Local script or executable** (file is copied to the bee and run there):
```bash
beemesh launch --host <hive-ip> --port 9000 --payload ./my_script.sh
beemesh launch --host <hive-ip> --port 9000 --payload /path/to/script.py
```

Results are printed on the hive once the bee finishes execution.

### Monitoring

The hive provides a simple dashboard to see connected bees and pending jobs:

```bash
beemesh monitor --host <hive-ip> --port 9000
```

<img src="./images/monitor.png" alt="Monitor Dashboard" width="600">

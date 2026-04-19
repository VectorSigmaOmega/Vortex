# Vortex - Minimalist Linux Container Runtime

Vortex is a lightweight Linux container runtime written in C++ that demonstrates the core principles of containerization: namespaces, control groups, and filesystem isolation.

## Features

- **Namespace Isolation**:
  - `PID`: Isolated process tree (container sees itself as PID 1).
  - `UTS`: Isolated hostname.
  - `Mount`: Isolated filesystem mounts.
  - `Network`: Isolated network stack (with loopback interface).
- **Filesystem Isolation**:
  - Uses `pivot_root` to securely change the root filesystem.
  - Automatically mounts `/proc` for process visibility.
- **Resource Control**:
  - Uses `Cgroups v2` to limit memory usage (default 100MB).
- **Alpine Linux Support**:
  - Designed to run with minimalist rootfs like Alpine.

## Architecture

Vortex works by following these steps:
1. **Clone**: Uses the `clone()` syscall with specific flags (`CLONE_NEWPID`, `CLONE_NEWUTS`, etc.) to create a new process in new namespaces.
2. **Cgroups**: The parent process creates a cgroup in `/sys/fs/cgroup/vortex` and adds the child PID to it, applying resource limits.
3. **Setup**: The child process:
   - Sets the hostname.
   - Bind-mounts the rootfs to itself (required for `pivot_root`).
   - Uses `pivot_root` to swap the root filesystem and unmounts the old root.
   - Mounts `/proc`.
   - Brings up the `lo` loopback interface.
4. **Exec**: Replaces the child process with the target command using `execvp`.

## How to Run

### Prerequisites

- Linux Kernel with Cgroups v2 support.
- Root privileges (or `CAP_SYS_ADMIN` capabilities).
- `g++` and `make`.

### 1. Build

```bash
make
```

### 2. Setup RootFS

Download and extract a minimalist Alpine RootFS:

```bash
./setup_rootfs.sh
```

### 3. Run a Container

```bash
sudo ./vortex run ./rootfs /bin/sh
```

Inside the container, you can verify isolation:

```bash
# Check PID
ps aux

# Check Hostname
hostname

# Check Filesystem
ls /
```

## Project Structure

- `src/vortex.cpp`: Main implementation.
- `Makefile`: Build instructions.
- `setup_rootfs.sh`: Script to prepare the container image.
- `rootfs/`: Directory containing the extracted Alpine Linux.

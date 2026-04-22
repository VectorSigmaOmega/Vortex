# 🌀 Vortex Container Runtime

![License](https://img.shields.io/badge/license-MIT-green.svg) 
![C++](https://img.shields.io/badge/C%2B%2B-20-orange.svg) 
![Platform](https://img.shields.io/badge/Platform-Linux-lightgrey.svg)
![Build](https://img.shields.io/badge/Build-Passing-brightgreen.svg)

Vortex is a high-performance, minimalist Linux container runtime built from the ground up in modern C++. It provides a "zero-dependency" approach to process isolation, resource limiting, and filesystem sandboxing.

```text
      __     __         _            
      \ \   / /__  _ __| |_ _____  __
       \ \ / / _ \| '__| __/ _ \ \/ /
        \ V / (_) | |  | ||  __/>  < 
         \_/ \___/|_|   \__\___/_/\_\
                                             
       - Isolated. Secure. Minimalist. -
```

## 🚀 Quick Start (Guided Mode)

Vortex features an interactive management console to simplify container operations.

```bash
make
./setup_rootfs.sh
sudo ./vortex
```

## 🛠 Features

- **Namespace Isolation**: Full UTS, PID, Mount, and Network stack isolation.
- **Filesystem Jail**: Securely jails processes using `pivot_root` (more robust than `chroot`).
- **Resource Limits**: Enforces hardware limits (CPU/Memory) via **Cgroups v2**.
- **Modern TUI**: A polished command-line interface with ANSI-coded status dashboards.
- **Zero Dependencies**: Requires only a modern C++ compiler and a standard Linux kernel.

## 📁 Developer Workflow: Importing Files

To run your own scripts or binaries inside a Vortex container, follow these steps:

### Method A: Using the Guided Menu (Recommended)
1. Run `sudo ./vortex`.
2. Select Option **4) Import (Side-load) file**.
3. Enter the path to your file on the host and its destination in the container.

### Method B: Manual Side-loading
Since the container's root is the `./rootfs` directory on your host, you can simply copy files:

```bash
# Copy a script to the container
cp my_script.sh ./rootfs/usr/bin/

# Launch the container and run the script
sudo ./vortex run ./rootfs /usr/bin/my_script.sh
```

## 🧪 Automated Testing

Vortex includes a professional-grade integration test suite to verify kernel isolation.

```bash
./tests/test_vortex.sh
```

## 📖 Architecture

For a deep dive into how Vortex interacts with the Linux kernel, see [ARCHITECTURE.md](./ARCHITECTURE.md).

## 📄 License

Distributed under the MIT License. See `LICENSE` for more information.

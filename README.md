# 🌀 Vortex Container Runtime

![License](https://img.shields.io/badge/license-MIT-green.svg) 
![C++](https://img.shields.io/badge/C%2B%2B-20-orange.svg) 
![Platform](https://img.shields.io/badge/Platform-Linux-lightgrey.svg)
![Build](https://img.shields.io/badge/Build-Passing-brightgreen.svg)

Vortex is a high-performance, minimalist Linux container runtime built from the ground up in modern C++. It provides a "zero-dependency" approach to process isolation, resource limiting, and filesystem sandboxing.

```text
      ____   ____            __                 
      \   \ /   /___________/  |_  ____ ___  ___
       \   Y   /  _ \_  __ \   __\/ __ \\  \/  /
        \     (  <_> )  | \/|  | \  ___/ >    < 
         \___/ \____/|__|   |__|  \___  >__/\_ \
                                      \/      \/

            - Isolated. Secure. Minimalist. -
```

<img width="808" height="510" alt="Screenshot 2026-04-23 154621" src="https://github.com/user-attachments/assets/85f9f6e5-df1d-4fae-98aa-d35ab49a5542" />


## 🚀 Quick Start (Guided Mode)

Vortex features an interactive management console to simplify container operations.

```bash
make
./setup_rootfs.sh
sudo ./vortex
```

## ⚙️ Configuration

Vortex settings can be adjusted on-the-fly via the **Settings Menu (Option 5)** or via CLI flags. These preferences are persisted in `.vortex_prefs`.

- **Network Mode**: 
  - **Isolated**: Air-gapped environment (default).
  - **Shared**: Grants the container access to the host's internet (useful for `apk add` commands).
- **Memory Limit**: Define a hard cap for the container's RAM usage. Vortex validates this against your physical `MemTotal` to prevent invalid configurations.

## 🛠 Features

- **Namespace Isolation**: Full UTS, PID, Mount, and Network stack isolation.
- **Dynamic Networking**: Toggle between **Isolated** and **Shared** modes without recompilation.
- **Filesystem Jail**: Securely jails processes using `pivot_root`.
- **Resource Control**: 
  - Configurable memory limits via **Cgroups v2**.
  - **Strict Enforcement**: Disables swap usage to ensure the memory limit is an absolute hard cap.
- **Persistent Settings**: Remembers your configuration across sessions.
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

Vortex includes an engineering-grade integration test suite to verify kernel isolation and feature enforcement.

```bash
# Verify base isolation (UTS, PID, Mount, Net)
./tests/test_vortex.sh

# Verify enhanced features (Memory limits, Shared networking)
./tests/test_vortex_enhanced.sh
```

## 🔥 Stress Testing: The OOM Killer

To see Vortex's resource control in action, use the provided memory stress test:

1. **Build the Hog**: `g++ -static examples/hog.cpp -o examples/hog`
2. **Import**: Select Option 4 in Vortex to move `examples/hog` to `/bin/hog`.
3. **Configure**: Set a 100MB limit in the Settings menu.
4. **Execute**: Launch the shell and run `hog`.
5. **Watch**: The Linux Kernel will instantly terminate the process when the limit is touched.

## 📖 Architecture

For a deep dive into how Vortex interacts with the Linux kernel, see [ARCHITECTURE.md](./ARCHITECTURE.md).

## 📄 License

Distributed under the MIT License. See `LICENSE` for more information.

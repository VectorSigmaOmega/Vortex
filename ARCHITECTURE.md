# Vortex Architecture

This document describes the internal mechanics of the Vortex container runtime.

## 1. Process Lifecycle

1.  **Entry**: The CLI parses the `run` command and the target `rootfs`.
2.  **Resource Reservation**: A Cgroup v2 directory is created in `/sys/fs/cgroup/vortex`. Memory limits are applied here before the child process gains momentum.
3.  **The Clone**: The `clone()` syscall is invoked with the following flags:
    *   `CLONE_NEWPID`: Process ID isolation.
    *   `CLONE_NEWUTS`: Hostname/Domain isolation.
    *   `CLONE_NEWNS`: Mount namespace isolation.
    *   `CLONE_NEWNET`: Network stack isolation.
4.  **The Jail (Child Process)**:
    *   **UTS**: `sethostname` is called to change the identity.
    *   **Mounts**: The root filesystem is bind-mounted. We use `pivot_root` to move the old root to a temporary directory and then unmount it. This is more secure than `chroot`.
    *   **Procfs**: A new `/proc` is mounted so that tools like `top` and `ps` work correctly within the namespace.
    *   **Network**: The loopback interface is brought up.
5.  **Execution**: `execvp` replaces the runtime code with the user's desired binary (e.g., `/bin/sh`).

## 2. Security Considerations

- **Capabilities**: Currently, the container runs with the inherited capabilities of the caller (usually root). In a production-grade runtime, we would use `libcap` to drop sensitive capabilities like `CAP_SYS_RAWIO` or `CAP_SYS_BOOT`.
- **Pivot_Root**: Unlike `chroot`, `pivot_root` changes the root of the entire mount namespace, making it significantly harder to "break out" of the container by using relative paths.

## 3. Resource Control (Cgroups v2)

Vortex utilizes the Unified Cgroup Hierarchy (v2). 
- **Path**: `/sys/fs/cgroup/vortex`
- **Controller**: `memory.max` is used to enforce a hard limit on memory consumption.
- **Cleanup**: The runtime attempts to remove the cgroup directory upon process exit to prevent "cgroup leaks."

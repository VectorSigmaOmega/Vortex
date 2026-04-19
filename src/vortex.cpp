#include <iostream>
#include <vector>
#include <string>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/mount.h>
#include <sched.h>
#include <cstring>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/syscall.h>

// Helper for pivot_root as it's not provided by glibc
int pivot_root(const char *new_root, const char *put_old) {
    return syscall(SYS_pivot_root, new_root, put_old);
}

struct ContainerConfig {
    std::string rootfs;
    std::string hostname;
    std::vector<std::string> args;
};

// Function to print usage
void print_usage(const char* prog_name) {
    std::cerr << "Usage: " << prog_name << " run <rootfs_path> <command> [args...]" << std::endl;
}

// Child process function
int container_main(void* arg) {
    auto config = static_cast<ContainerConfig*>(arg);
    
    // 1. Set hostname
    if (sethostname(config->hostname.c_str(), config->hostname.length()) == -1) {
        perror("sethostname");
        return 1;
    }

    // 2. Prepare filesystem
    if (mount(NULL, "/", NULL, MS_REC | MS_PRIVATE, NULL) == -1) {
        perror("mount / MS_PRIVATE");
        return 1;
    }

    if (mount(config->rootfs.c_str(), config->rootfs.c_str(), NULL, MS_BIND | MS_REC, NULL) == -1) {
        perror("mount bind rootfs");
        return 1;
    }

    std::string old_root_path = config->rootfs + "/.old_root";
    mkdir(old_root_path.c_str(), 0700);

    if (pivot_root(config->rootfs.c_str(), old_root_path.c_str()) == -1) {
        perror("pivot_root");
        return 1;
    }

    if (chdir("/") == -1) {
        perror("chdir /");
        return 1;
    }

    if (umount2("/.old_root", MNT_DETACH) == -1) {
        perror("umount .old_root");
        return 1;
    }
    rmdir("/.old_root");

    if (mount("proc", "/proc", "proc", 0, NULL) == -1) {
        perror("mount proc");
        return 1;
    }

    // Bring up loopback interface
    if (system("ip link set lo up") == -1) {
        perror("system ip link set lo up");
    }

    // 3. Execute command
    char** argv = new char*[config->args.size() + 1];
    for (size_t i = 0; i < config->args.size(); ++i) {
        argv[i] = const_cast<char*>(config->args[i].c_str());
    }
    argv[config->args.size()] = nullptr;

    if (execvp(argv[0], argv) == -1) {
        perror("execvp");
        return 1;
    }

    return 0;
}

void setup_cgroups(pid_t child_pid) {
    std::string cgroup_base = "/sys/fs/cgroup/vortex";
    std::cout << "Setting up cgroup: " << cgroup_base << " for PID " << child_pid << std::endl;
    if (mkdir(cgroup_base.c_str(), 0755) == -1 && errno != EEXIST) {
        perror("mkdir cgroup");
        return;
    }

    // Add process to cgroup
    int fd = open((cgroup_base + "/cgroup.procs").c_str(), O_WRONLY);
    if (fd != -1) {
        std::string pid_str = std::to_string(child_pid);
        if (write(fd, pid_str.c_str(), pid_str.length()) == -1) {
            perror("write cgroup.procs");
        }
        close(fd);
    }

    // Set memory limit (100MB)
    fd = open((cgroup_base + "/memory.max").c_str(), O_WRONLY);
    if (fd != -1) {
        const char* limit = "104857600";
        if (write(fd, limit, strlen(limit)) == -1) {
            perror("write memory.max");
        }
        close(fd);
    }
}

void cleanup_cgroups() {
    std::string cgroup_base = "/sys/fs/cgroup/vortex";
    rmdir(cgroup_base.c_str());
}

int main(int argc, char** argv) {
    if (argc < 4 || std::string(argv[1]) != "run") {
        print_usage(argv[0]);
        return 1;
    }

    ContainerConfig config;
    config.rootfs = argv[2];
    config.hostname = "vortex-container";
    for (int i = 3; i < argc; ++i) {
        config.args.push_back(argv[i]);
    }

    const int STACK_SIZE = 1024 * 1024;
    char* stack = new char[STACK_SIZE];
    char* stack_top = stack + STACK_SIZE;

    int flags = CLONE_NEWPID | CLONE_NEWUTS | CLONE_NEWNS | CLONE_NEWNET | SIGCHLD;

    pid_t pid = clone(container_main, stack_top, flags, &config);

    if (pid == -1) {
        perror("clone");
        delete[] stack;
        return 1;
    }

    setup_cgroups(pid);

    waitpid(pid, nullptr, 0);
    
    cleanup_cgroups();
    delete[] stack;

    return 0;
}

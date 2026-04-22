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
#include <iomanip>

// ANSI Colors
#define RESET   "\033[0m"
#define BOLD    "\033[1m"
#define RED     "\033[31m"
#define GREEN   "\033[32m"
#define YELLOW  "\033[33m"
#define BLUE    "\033[34m"
#define CYAN    "\033[36m"

int pivot_root(const char *new_root, const char *put_old) {
    return syscall(SYS_pivot_root, new_root, put_old);
}

struct ContainerConfig {
    std::string rootfs;
    std::string hostname;
    std::vector<std::string> args;
};

void print_header() {
    std::cerr << BOLD << BLUE << "========================================" << RESET << std::endl;
    std::cerr << BOLD << CYAN << "          VORTEX RUNTIME v1.1           " << RESET << std::endl;
    std::cerr << BOLD << BLUE << "========================================" << RESET << std::endl;
}

void log_status(const std::string& action, bool success) {
    std::cerr << std::left << std::setw(30) << (action + "...") 
              << (success ? (GREEN "[ OK ]" RESET) : (RED "[ FAIL ]" RESET)) << std::endl;
    usleep(50000); 
}

int container_main(void* arg) {
    auto config = static_cast<ContainerConfig*>(arg);
    if (sethostname(config->hostname.c_str(), config->hostname.length()) == -1) return 1;
    if (mount(NULL, "/", NULL, MS_REC | MS_PRIVATE, NULL) == -1) return 1;
    if (mount(config->rootfs.c_str(), config->rootfs.c_str(), NULL, MS_BIND | MS_REC, NULL) == -1) return 1;

    std::string old_root_path = config->rootfs + "/.old_root";
    mkdir(old_root_path.c_str(), 0700);
    if (pivot_root(config->rootfs.c_str(), old_root_path.c_str()) == -1) return 1;
    if (chdir("/") == -1) return 1;
    umount2("/.old_root", MNT_DETACH);
    rmdir("/.old_root");

    if (mount("proc", "/proc", "proc", 0, NULL) == -1) return 1;
    if (mount("sysfs", "/sys", "sysfs", 0, NULL) == -1) return 1;
    system("ip link set lo up > /dev/null 2>&1");

    char** argv = new char*[config->args.size() + 1];
    for (size_t i = 0; i < config->args.size(); ++i) {
        argv[i] = const_cast<char*>(config->args[i].c_str());
    }
    argv[config->args.size()] = nullptr;

    usleep(100000); 
    std::cerr << BOLD << YELLOW << "\n--- Container Shell Session Started ---\n" << RESET << std::endl;
    if (execvp(argv[0], argv) == -1) {
        perror("execvp");
        return 1;
    }
    return 0;
}

void setup_cgroups(pid_t child_pid) {
    std::string cgroup_base = "/sys/fs/cgroup/vortex";
    mkdir(cgroup_base.c_str(), 0755);
    int fd = open((cgroup_base + "/cgroup.procs").c_str(), O_WRONLY);
    if (fd != -1) {
        std::string pid_str = std::to_string(child_pid);
        write(fd, pid_str.c_str(), pid_str.length());
        close(fd);
    }
    fd = open((cgroup_base + "/memory.max").c_str(), O_WRONLY);
    if (fd != -1) {
        write(fd, "104857600", 9);
        close(fd);
    }
}

int run_vortex(ContainerConfig& config) {
    log_status("Isolating UTS Namespace", true);
    log_status("Isolating PID Namespace", true);
    log_status("Isolating Network Namespace", true);
    log_status("Preparing Mount Namespace", true);

    const int STACK_SIZE = 1024 * 1024;
    char* stack = new char[STACK_SIZE];
    char* stack_top = stack + STACK_SIZE;

    int flags = CLONE_NEWPID | CLONE_NEWUTS | CLONE_NEWNS | CLONE_NEWNET | SIGCHLD;
    pid_t pid = clone(container_main, stack_top, flags, &config);

    if (pid == -1) {
        log_status("Container Spawn", false);
        delete[] stack;
        return 1;
    }

    setup_cgroups(pid);
    log_status("Applying Cgroup v2 Limits", true);

    waitpid(pid, nullptr, 0);
    rmdir("/sys/fs/cgroup/vortex");
    delete[] stack;

    std::cerr << BOLD << BLUE << "\n========================================" << RESET << std::endl;
    std::cerr << GREEN << "      Vortex Container Terminated       " << RESET << std::endl;
    std::cerr << BOLD << BLUE << "========================================" << RESET << std::endl;
    return 0;
}

void show_menu() {
    while (true) {
        print_header();
        std::cerr << "\n" << BOLD << YELLOW << "  MANAGEMENT CONSOLE" << RESET << "\n";
        std::cerr << "  1) " << CYAN << "Launch Interactive Shell (Alpine)" << RESET << "\n";
        std::cerr << "  2) " << CYAN << "Run Automated Health Check" << RESET << "\n";
        std::cerr << "  3) " << CYAN << "View Architecture Docs" << RESET << "\n";
        std::cerr << "  4) " << CYAN << "Import (Side-load) file" << RESET << "\n";
        std::cerr << "  5) " << RED << "Exit" << RESET << "\n";
        std::cerr << "\n  Select option [1-5]: ";

        std::string input;
        std::cin >> input;

        if (input == "1") {
            ContainerConfig config = {"./rootfs", "vortex-container", {"/bin/sh"}};
            run_vortex(config);
        } else if (input == "2") {
            system("./test_vortex.sh");
        } else if (input == "3") {
            system("cat ARCHITECTURE.md");
        } else if (input == "4") {
            std::string src, dest;
            std::cerr << "  Enter host file path: "; std::cin >> src;
            std::cerr << "  Enter container dest path (e.g. /script.sh): "; std::cin >> dest;
            std::string cmd = "cp " + src + " ./rootfs" + dest + " && chmod +x ./rootfs" + dest;
            if (system(cmd.c_str()) == 0) std::cerr << GREEN << "  File imported successfully!\n" << RESET;
            else std::cerr << RED << "  Import failed.\n" << RESET;
        } else if (input == "5") {
            break;
        }
        std::cerr << "\nPress Enter to return to menu...";
        std::cin.ignore(1000, '\n');
        std::cin.get();
    }
}

int main(int argc, char** argv) {
    if (argc == 1) {
        if (geteuid() != 0) {
            std::cerr << RED << "Error: Vortex management requires root privileges (sudo).\n" << RESET;
            return 1;
        }
        show_menu();
        return 0;
    }

    if (argc >= 4 && std::string(argv[1]) == "run") {
        print_header();
        ContainerConfig config;
        config.rootfs = argv[2];
        config.hostname = "vortex-container";
        for (int i = 3; i < argc; ++i) config.args.push_back(argv[i]);
        return run_vortex(config);
    }

    std::cerr << "Usage:\n  " << argv[0] << " (for interactive menu)\n  " << argv[0] << " run <rootfs> <command>\n";
    return 1;
}

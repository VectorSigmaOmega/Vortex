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
#include <termios.h>
#include <signal.h>
#include <fstream>

// ANSI Colors
#define RESET   "\033[0m"
#define BOLD    "\033[1m"
#define RED     "\033[31m"
#define GREEN   "\033[32m"
#define YELLOW  "\033[33m"
#define BLUE    "\033[34m"
#define CYAN    "\033[36m"

struct ContainerConfig {
    std::string rootfs = "./rootfs";
    std::string hostname = "vortex-container";
    std::vector<std::string> args;
    long memory_limit_mb = 100;
    bool network_isolated = true;
};

// Global config for the menu
ContainerConfig global_prefs;
const std::string PREFS_FILE = ".vortex_prefs";

void save_prefs() {
    std::ofstream file(PREFS_FILE);
    if (file.is_open()) {
        file << global_prefs.memory_limit_mb << "\n";
        file << global_prefs.network_isolated << "\n";
        file.close();
    }
}

void load_prefs() {
    std::ifstream file(PREFS_FILE);
    if (file.is_open()) {
        long mem;
        bool net;
        if (file >> mem >> net) {
            global_prefs.memory_limit_mb = mem;
            global_prefs.network_isolated = net;
        }
        file.close();
    }
}

int pivot_root(const char *new_root, const char *put_old) {
    return syscall(SYS_pivot_root, new_root, put_old);
}

long get_system_ram_mb() {
    std::ifstream meminfo("/proc/meminfo");
    std::string line;
    if (meminfo.is_open()) {
        while (std::getline(meminfo, line)) {
            if (line.find("MemTotal:") == 0) {
                try {
                    size_t first_digit = line.find_first_of("0123456789");
                    size_t last_digit = line.find_last_of("0123456789");
                    if (first_digit != std::string::npos && last_digit != std::string::npos) {
                        long kb = std::stol(line.substr(first_digit, last_digit - first_digit + 1));
                        return kb / 1024;
                    }
                } catch (...) {}
            }
        }
    }
    return 4096;
}

void clear_screen() {
    auto res = write(STDERR_FILENO, "\033[2J\033[1;1H", 11);
    (void)res;
}

void print_logo() {
    std::cerr << BOLD << BLUE <<
    "   ____   ____            __                  \n"
    "   \\   \\ /   /___________/  |_  ____ ___  ___ \n"
    "    \\   Y   /  _ \\_  __ \\   __\\/ __ \\  \\/  / \n"
    "     \\     (  <_> )  | \\/|  | \\  ___/ >    < \n"
    "      \\___/ \\____/|__|   |__|  \\___  >__/\\_ \\ \n"
    "                                   \\/      \\/   \n" <<RESET;
}

void print_header() {
    std::cerr << BOLD << BLUE << "  ================================================" << RESET << std::endl;
    std::cerr << BOLD << CYAN << "               VORTEX RUNTIME v1.2                " << RESET << std::endl;
    std::cerr << BOLD << BLUE << "  ================================================" << RESET << std::endl;
}

void render_ui(const std::string& title = "MANAGEMENT CONSOLE") {
    clear_screen();
    print_logo();
    print_header();
    std::cerr << "\n  " << BOLD << YELLOW << title << RESET << "\n";
}

void log_status(const std::string& action, bool success) {
    std::cerr << "  " << std::left << std::setw(30) << (action + "...") 
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
    
    if (config->network_isolated) {
        auto res = system("ip link set lo up > /dev/null 2>&1");
        (void)res;
    }

    char** argv = new char*[config->args.size() + 1];
    for (size_t i = 0; i < config->args.size(); ++i) {
        argv[i] = const_cast<char*>(config->args[i].c_str());
    }
    argv[config->args.size()] = nullptr;

    usleep(100000); 
    std::cerr << BOLD << YELLOW << "\n  --- Container Shell Session Started ---\n" << RESET << std::endl;
    if (execvp(argv[0], argv) == -1) {
        perror("execvp");
        return 1;
    }
    return 0;
}

void setup_cgroups(pid_t child_pid, long limit_mb) {
    int fd_sub = open("/sys/fs/cgroup/cgroup.subtree_control", O_WRONLY);
    if (fd_sub != -1) {
        auto res_sub = write(fd_sub, "+memory", 7);
        (void)res_sub;
        close(fd_sub);
    }

    std::string cgroup_base = "/sys/fs/cgroup/vortex";
    auto res_mkdir = mkdir(cgroup_base.c_str(), 0755); (void)res_mkdir;
    int fd = open((cgroup_base + "/cgroup.procs").c_str(), O_WRONLY);
    if (fd != -1) {
        std::string pid_str = std::to_string(child_pid);
        auto res_w = write(fd, pid_str.c_str(), pid_str.length()); (void)res_w;
        close(fd);
    }
    fd = open((cgroup_base + "/memory.max").c_str(), O_WRONLY);
    if (fd != -1) {
        std::string limit_str = std::to_string(limit_mb * 1024 * 1024);
        auto res_w = write(fd, limit_str.c_str(), limit_str.length()); (void)res_w;
        close(fd);
    }
    fd = open((cgroup_base + "/memory.swap.max").c_str(), O_WRONLY);
    if (fd != -1) {
        auto res_w = write(fd, "0", 1); (void)res_w;
        close(fd);
    }
}

int run_vortex(ContainerConfig& config, bool interactive = true) {
    if (interactive) {
        render_ui("INITIALIZING CONTAINER");
    }
    
    log_status("Isolating UTS Namespace", true);
    log_status("Isolating PID Namespace", true);
    log_status(config.network_isolated ? "Isolating Network Namespace" : "Shared Network Mode", true);
    log_status("Preparing Mount Namespace", true);

    struct termios orig_termios;
    tcgetattr(STDIN_FILENO, &orig_termios);
    pid_t orig_pgrp = tcgetpgrp(STDIN_FILENO);

    const int STACK_SIZE = 1024 * 1024;
    char* stack = new char[STACK_SIZE];
    char* stack_top = stack + STACK_SIZE;

    int flags = CLONE_NEWPID | CLONE_NEWUTS | CLONE_NEWNS | SIGCHLD;
    if (config.network_isolated) flags |= CLONE_NEWNET;

    pid_t pid = clone(container_main, stack_top, flags, &config);
    if (pid == -1) {
        log_status("Container Spawn", false);
        delete[] stack;
        return 1;
    }

    setup_cgroups(pid, config.memory_limit_mb);
    log_status("Applying Limits (" + std::to_string(config.memory_limit_mb) + "MB)", true);

    int status;
    waitpid(pid, &status, 0);

    (void)rmdir("/sys/fs/cgroup/vortex"); 
    delete[] stack;

    signal(SIGTTOU, SIG_IGN);
    tcsetpgrp(STDIN_FILENO, orig_pgrp);
    tcsetattr(STDIN_FILENO, TCSANOW, &orig_termios);
    signal(SIGTTOU, SIG_DFL);

    return WIFEXITED(status) ? WEXITSTATUS(status) : 128 + WTERMSIG(status);
}

void show_settings() {
    long max_ram = get_system_ram_mb();
    while (true) {
        render_ui("VORTEX SETTINGS");
        std::cerr << "  1) Memory Limit:   " << BOLD << YELLOW << global_prefs.memory_limit_mb << " MB" << RESET << "\n";
        std::cerr << "  2) Network Mode:   " << BOLD << YELLOW << (global_prefs.network_isolated ? "ISOLATED" : "SHARED (Internet)") << RESET << "\n";
        std::cerr << "  3) Return to Menu\n";
        std::cerr << "\n  Select option [1-3]: ";

        std::string input;
        if (!std::getline(std::cin, input)) break;

        if (input == "1") {
            std::cerr << "  Enter new limit in MB (System Max: " << max_ram << "): ";
            std::string val; std::getline(std::cin, val);
            try {
                long new_limit = std::stol(val);
                if (new_limit > 0 && new_limit <= max_ram) {
                    global_prefs.memory_limit_mb = new_limit;
                    save_prefs();
                } else {
                    std::cerr << RED << "  [!] Invalid limit. Must be between 1 and " << max_ram << RESET << "\n";
                    usleep(1000000);
                }
            } catch (...) {}
        } else if (input == "2") {
            global_prefs.network_isolated = !global_prefs.network_isolated;
            save_prefs();
        } else if (input == "3") {
            break;
        } else {
            std::cerr << RED << "  [!] Invalid option. Please select 1-3." << RESET << std::endl;
            usleep(800000);
            continue;
        }
    }
}

void show_menu() {
    while (true) {
        render_ui();
        std::cerr << "  1) " << CYAN << "Launch Interactive Shell (Alpine)" << RESET << "\n";
        std::cerr << "  2) " << CYAN << "Run Automated Health Check" << RESET << "\n";
        std::cerr << "  3) " << CYAN << "View Architecture Docs" << RESET << "\n";
        std::cerr << "  4) " << CYAN << "Import (Side-load) file" << RESET << "\n";
        std::cerr << "  5) " << YELLOW << "Vortex Settings" << RESET << "\n";
        std::cerr << "  6) " << RED << "Exit" << RESET << "\n";
        std::cerr << "\n  Select option [1-6]: ";

        std::string input;
        if (!std::getline(std::cin, input)) break;

        if (input == "1") {
            global_prefs.args = {"/bin/sh"};
            run_vortex(global_prefs);
            continue; 
        } else if (input == "2") {
            render_ui("SYSTEM HEALTH CHECK");
            auto res = system("./tests/test_vortex.sh"); (void)res;
        } else if (input == "3") {
            render_ui("ARCHITECTURE DOCUMENTATION");
            std::cerr << "\n";
            auto res = system("cat ARCHITECTURE.md | sed 's/^/  /'"); (void)res;
        } else if (input == "4") {
            render_ui("FILE IMPORT UTILITY");
            std::cerr << "  Example Host Path: " << CYAN << "./my_script.sh" << RESET << "\n";
            std::cerr << "  Example Dest Path: " << CYAN << "/bin/my_script.sh" << RESET << "\n";
            std::cerr << "  (Type 'q' to return to menu)\n\n";
            
            std::string src, dest;
            std::cerr << "  Enter host file path: "; if (!std::getline(std::cin, src) || src == "q" || src.empty()) continue;
            std::cerr << "  Enter container dest path: "; if (!std::getline(std::cin, dest) || dest == "q" || dest.empty()) continue;
            
            std::string cmd = "cp " + src + " ./rootfs" + dest + " && chmod +x ./rootfs" + dest + " 2>/dev/null";
            if (system(cmd.c_str()) == 0) {
                std::cerr << "\n" << GREEN << "  [SUCCESS] " << RESET << "File imported and made executable.\n";
            } else {
                std::cerr << "\n" << RED << "  [ERROR] " << RESET << "Could not copy file. Check paths.\n";
            }
        } else if (input == "5") {
            show_settings();
            continue;
        } else if (input == "6") {
            break;
        } else {
            std::cerr << RED << "  [!] Invalid option. Please select 1-6." << RESET << std::endl;
            usleep(800000);
            continue;
        }
        
        std::cerr << "\n  " << BOLD << "Press Enter to return to menu..." << RESET;
        std::cin.clear();
        std::string dummy;
        std::getline(std::cin, dummy);
    }
}

int main(int argc, char** argv) {
    load_prefs();
    bool is_tty = isatty(STDIN_FILENO);

    if (argc == 1) {
        if (geteuid() != 0) {
            std::cerr << RED << "  Error: Vortex requires root privileges (sudo).\n" << RESET;
            return 1;
        }
        show_menu();
        return 0;
    }

    ContainerConfig config = global_prefs;
    int arg_offset = 1;
    while (arg_offset < argc && argv[arg_offset][0] == '-') {
        std::string flag = argv[arg_offset];
        if (flag == "--shared-net") config.network_isolated = false;
        else if (flag == "--isolated-net") config.network_isolated = true;
        else if (flag == "--memory") {
            if (arg_offset + 1 < argc) config.memory_limit_mb = std::stol(argv[++arg_offset]);
        }
        arg_offset++;
    }

    if (arg_offset < argc && std::string(argv[arg_offset]) == "run") {
        arg_offset++;
        if (arg_offset < argc) {
            config.rootfs = argv[arg_offset++];
            while (arg_offset < argc) config.args.push_back(argv[arg_offset++]);
            return run_vortex(config, is_tty);
        }
    }

    std::cerr << "Usage:\n  " << argv[0] << " (for menu)\n  " << argv[0] << " [options] run <rootfs> <cmd>\n";
    std::cerr << "Options:\n  --shared-net / --isolated-net\n  --memory <MB>\n";
    return 1;
}

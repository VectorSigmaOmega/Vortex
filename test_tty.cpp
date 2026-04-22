#include <iostream>
#include <termios.h>
#include <unistd.h>
#include <sys/wait.h>

int main() {
    struct termios orig;
    tcgetattr(STDIN_FILENO, &orig);
    pid_t pid = fork();
    if (pid == 0) {
        execlp("/bin/sh", "sh", nullptr);
        return 1;
    }
    waitpid(pid, nullptr, 0);
    tcsetattr(STDIN_FILENO, TCSANOW, &orig);
    std::cout << "Done.\n";
    return 0;
}

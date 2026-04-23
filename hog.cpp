#include <iostream>
#include <vector>
#include <cstring>
#include <unistd.h>

int main() {
    std::vector<char*> memory;
    std::cout << "Starting Aggressive Memory Hog..." << std::endl;

    try {
        while (true) {
            char* block = new char[1024 * 1024];
            memset(block, 0, 1024 * 1024);
            memory.push_back(block);
            if (memory.size() % 10 == 0) {
                std::cout << "Consumed: " << memory.size() << " MB" << std::endl;
            }
        }
    } catch (const std::bad_alloc& e) {
        std::cerr << "Memory allocation failed: " << e.what() << std::endl;
    }
    return 0;
}

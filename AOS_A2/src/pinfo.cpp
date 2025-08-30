#include "pinfo.h"
#include <iostream>
#include <fstream>
#include <unistd.h>

void pinfo(pid_t pid) {
    if (pid == 0) pid = getpid();
    std::cout << "pid -- " << pid << "\n";

#if defined(__linux__)
    std::string path = "/proc/" + std::to_string(pid) + "/stat";
    std::ifstream fin(path);
    if (fin) {
        std::string data;
        std::getline(fin, data);
        std::cout << "stat -- " << data << "\n";
    }
#elif defined(__APPLE__)
    std::cout << "pinfo not fully supported on macOS\n";
#endif
}
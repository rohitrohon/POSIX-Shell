#include "pinfo.h"
#include <iostream>
#include <fstream>
#include <unistd.h>
#include <string>
#include <sys/types.h>
#include <sys/sysctl.h>
#include <libproc.h>
#include <fcntl.h>

void pinfo(pid_t pid) {
    if (pid == 0) {
        pid = getpid();
    }

    // Get process information
    struct proc_taskinfo task_info;
    int status = proc_pidinfo(pid, PROC_PIDTASKINFO, 0, &task_info, sizeof(task_info));
    
    if (status <= 0) {
        std::cerr << "Error: Process " << pid << " not found.\n";
        return;
    }

    // Get process path
    char path_buffer[PROC_PIDPATHINFO_MAXSIZE];
    if (proc_pidpath(pid, path_buffer, sizeof(path_buffer)) <= 0) {
        std::cerr << "Error: Cannot get process path\n";
        return;
    }

    // Get process status
    char status_char = 'U'; // Unknown by default
    bool is_foreground = false;

    struct proc_bsdinfo proc_info;
    if (proc_pidinfo(pid, PROC_PIDTBSDINFO, 0, &proc_info, sizeof(proc_info)) > 0) {
        // Convert BSD status to our format
        switch (proc_info.pbi_status) {
            case SRUN:
                status_char = 'R';
                break;
            case SSLEEP:
                status_char = 'S';
                break;
            case SSTOP:
                status_char = 'T';
                break;
            case SZOMB:
                status_char = 'Z';
                break;
        }

        // Check if process is in foreground
        pid_t fg_pid = tcgetpgrp(STDIN_FILENO);
        if (fg_pid > 0 && proc_info.pbi_pgid == fg_pid) {
            is_foreground = true;
        }
    }

    // Print process information in the required format
    std::cout << "pid -- " << pid << std::endl;
    std::cout << "Process Status -- " << status_char << (is_foreground ? "+" : "") << std::endl;
    std::cout << "memory -- " << task_info.pti_virtual_size << " {Virtual Memory}" << std::endl;
    std::cout << "Executable Path -- " << path_buffer << std::endl;
}
#include "signals.h"
#include "jobs.h"
#include <csignal>
#include <iostream>
#include <sys/wait.h>
#include <unistd.h>

pid_t fg_pid = -1;
std::string fg_cmd = "";

static void sigint_handler(int) {
    if (fg_pid > 0) kill(fg_pid, SIGINT);
    else std::cout << "\n";
}

static void sigtstp_handler(int) {
    if (fg_pid > 0) {
        kill(fg_pid, SIGTSTP);
        add_job(fg_pid, fg_cmd, false);
        std::cout << "\n[" << fg_pid << "] Stopped " << fg_cmd << "\n";
        fg_pid = -1;
        fg_cmd.clear();
    } else std::cout << "\n";
}

static void sigchld_handler(int) {
    int status;
    pid_t pid;
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        remove_job(pid);
    }
}

void init_signal_handlers() {
    signal(SIGINT, sigint_handler);
    signal(SIGTSTP, sigtstp_handler);
    signal(SIGCHLD, sigchld_handler);
}
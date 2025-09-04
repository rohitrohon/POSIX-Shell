#include "common.h"
#include <csignal>
#include <iostream>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

using namespace std;
pid_t FG_PGID = 0;       // Foreground process group ID

// --- SIGINT (Ctrl+C) handler ---
void sigint_handler(int) {
    if (FG_PGID != 0) {
        // Send SIGINT to the whole foreground process group
        kill(-FG_PGID, SIGINT);
        cout << "\n";
    } 
}

// --- SIGTSTP (Ctrl+Z) handler ---
void sigtstp_handler(int) {
    if (FG_PGID != 0) {
        // Send SIGTSTP to the whole foreground process group
        cout << "\n[Stopped] " << FG_PGID << "\n";
        kill(-FG_PGID, SIGTSTP);
    } 
}

// --- Setup function called from main ---
void install_shell_signal_handlers() {
    struct sigaction sa;

    // Handle Ctrl+C
    sa.sa_handler = sigint_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    sigaction(SIGINT, &sa, nullptr);

    // Handle Ctrl+Z
    sa.sa_handler = sigtstp_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    sigaction(SIGTSTP, &sa, nullptr);

    // Ignore terminal job control signals so shell isn’t stopped
    signal(SIGTTOU, SIG_IGN);
    signal(SIGTTIN, SIG_IGN); //
}
#ifndef SIGNALS_H
#define SIGNALS_H

#include <string>
#include <vector>
#include <sys/types.h>

using namespace std;

// Global vars
extern string SHELL_HOME;
extern pid_t FG_PGID;  

// Functions
void sigint_handler(int);
void sigtstp_handler(int);
void install_shell_signal_handlers();

#endif
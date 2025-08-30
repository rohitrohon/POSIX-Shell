#ifndef SIGNALS_H
#define SIGNALS_H
#include <sys/types.h>
#include <string>

extern pid_t fg_pid;
extern std::string fg_cmd;

void init_signal_handlers();

#endif
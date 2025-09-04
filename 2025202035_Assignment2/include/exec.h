
#ifndef EXEC_H
#define EXEC_H
#include "parser.h"
void run_parsed(Parsed& p);
std::string build_cmd_string(const Parsed& p);
#endif

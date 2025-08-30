#ifndef HISTORY_H
#define HISTORY_H
#include <string>
void load_history();
void save_history();
void add_history(const std::string& cmd);
void show_history(int n);
#endif

#ifndef ARROW_H
#define ARROW_H
#include <string>
#include <vector>
std::string read_input_line();
void load_history();
void save_history();
const std::vector<std::string>& get_history();
#endif

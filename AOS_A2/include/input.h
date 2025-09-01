#ifndef INPUT_H
#define INPUT_H

#include <string>

// Function to read a line with arrow key support
std::string read_line_with_history();

// Function to set up terminal for raw input
void setup_raw_mode();

// Function to restore terminal settings
void restore_terminal();

#endif // INPUT_H

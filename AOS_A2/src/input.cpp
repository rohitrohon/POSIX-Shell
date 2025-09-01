#include "input.h"
#include "history.h"
#include <iostream>
#include <termios.h>
#include <unistd.h>
#include <cstring>
#include <vector>

static struct termios orig_termios;  // Save original terminal settings
static std::vector<std::string> history;
static size_t history_index = 0;
static std::string current_input;
static bool history_navigation = false;

void setup_raw_mode() {
    tcgetattr(STDIN_FILENO, &orig_termios);
    struct termios raw = orig_termios;
    raw.c_lflag &= ~(ICANON | ECHO);  // Disable canonical mode and echo
    raw.c_cc[VMIN] = 1;  // Read one character at a time
    raw.c_cc[VTIME] = 0;  // No timeout
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

void restore_terminal() {
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
}

static void clear_line() {
    std::cout << "\r\033[K";  // Clear line from cursor to end
    std::cout.flush();
}

static void refresh_line(const std::string& prompt, const std::string& line) {
    clear_line();
    std::cout << prompt << line;
    std::cout.flush();
}

std::string read_line_with_history() {
    const std::string prompt = "mysh> ";  // You might want to get this from your prompt module
    std::string line;
    char c;
    bool reading = true;
    int escape_seq = 0;
    char escape_buffer[3] = {0};

    // Initialize or reset state
    if (!history_navigation) {
        current_input = "";
        history_index = history.size();
    }

    std::cout << prompt;
    std::cout.flush();

    while (reading && read(STDIN_FILENO, &c, 1) == 1) {
        if (escape_seq > 0) {
            escape_buffer[escape_seq - 1] = c;
            escape_seq++;

            if (escape_seq == 3) {
                escape_seq = 0;
                if (escape_buffer[0] == '[') {
                    if (escape_buffer[1] == 'A') {  // Up arrow
                        if (history_index > 0) {
                            if (!history_navigation) {
                                current_input = line;  // Save current input first time
                                history_navigation = true;
                            }
                            history_index--;
                            line = history[history_index];
                            refresh_line(prompt, line);
                        }
                    }
                    else if (escape_buffer[1] == 'B') {  // Down arrow
                        if (history_navigation) {
                            if (history_index < history.size() - 1) {
                                history_index++;
                                line = history[history_index];
                            }
                            else {
                                history_index = history.size();
                                line = current_input;
                                history_navigation = false;
                            }
                            refresh_line(prompt, line);
                        }
                    }
                }
            }
            continue;
        }

        switch (c) {
            case '\x1b':  // ESC character
                escape_seq = 1;
                break;

            case '\n':    // Enter
            case '\r':
                std::cout << "\n";
                reading = false;
                history_navigation = false;
                break;

            case 127:    // Backspace
            case '\b':
                if (!line.empty()) {
                    line.pop_back();
                    refresh_line(prompt, line);
                }
                break;

            case 3:     // Ctrl-C
                std::cout << "^C\n";
                line.clear();
                reading = false;
                break;

            case 4:     // Ctrl-D
                if (line.empty()) {
                    std::cout << "\n";
                    return "exit";
                }
                break;

            default:
                if (!iscntrl(c)) {  // Only add printable characters
                    line += c;
                    std::cout << c;
                    std::cout.flush();
                }
                break;
        }
    }

    return line;
}

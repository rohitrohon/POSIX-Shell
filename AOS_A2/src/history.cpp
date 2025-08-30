#include "history.h"
#include <vector>
#include <string>
#include <fstream>
#include <iostream>
#include <cstdlib>

static std::vector<std::string> history;
static const std::string histfile = "/tmp/mysh_history.txt";

void load_history() {
    history.clear();
    std::ifstream fin(histfile);
    std::string line;
    while (std::getline(fin, line)) {
        if (!line.empty()) history.push_back(line);
    }
}

void save_history() {
    std::ofstream fout(histfile);
    for (auto& h : history) fout << h << "\n";
}

void add_history(const std::string& cmd) {
    history.push_back(cmd);
    if (history.size() > 100) history.erase(history.begin());
}

void show_history(int n) {
    int hist_size = static_cast<int>(history.size());
    int start = (hist_size > n) ? hist_size - n : 0;
    for (int i = start; i < hist_size; i++)
        std::cout << history[i] << "\n";
}

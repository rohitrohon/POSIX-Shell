#include "common.h"
#include <algorithm>
#include <cctype>
#include <string>
#include <vector>
using namespace std;
// Keep only utility functions here, no signal handlers.

string tildeify(const string& p) {
    if (p == "/") return "";
    if (!SHELL_HOME.empty() && p.rfind(SHELL_HOME, 0) == 0) {
        if (p.size() == SHELL_HOME.size()) 
            return "~";
        if (p[SHELL_HOME.size()] == '/') 
            return "~" + p.substr(SHELL_HOME.size());
    }
    return p;
}

static inline bool is_space(char c) {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

string trim(const string& s) {
    size_t i = 0, j = s.size();
    while (i < j && is_space(s[i])) ++i;
    while (j > i && is_space(s[j - 1])) --j;
    return s.substr(i, j - i);
}

vector<string> split_simple(const string& s, char delim) {
    vector<string> out;
    size_t start = 0;
    for (size_t i = 0; i <= s.size(); ++i) {
        if (i == s.size() || s[i] == delim) {
            out.push_back(s.substr(start, i - start));
            start = i + 1;
        }
    }
    return out;
}
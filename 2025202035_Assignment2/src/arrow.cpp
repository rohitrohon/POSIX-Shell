#include "arrow.h"
#include "prompt.h"
#include "common.h"
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <cstdio>
#include <cstdlib>
#include <cctype>
#include <vector>
#include <string>
#include <fstream>
#include <iostream>
#include <algorithm>
#include <readline/readline.h>
#include <readline/history.h>

using namespace std;

// Declare readline globals (provided by libreadline)
extern int rl_catch_signals;
extern int rl_catch_sigwinch;

// ---------- History storage ----------
static vector<string> history_store;
static const string histfile =
    string(getenv("HOME") ? getenv("HOME") : "") + "/.mysh_history_child";

const vector<string>& get_history() { return history_store; }

void load_history() {
    history_store.clear();
    ifstream fin(histfile);
    string line;
    while (getline(fin, line)) {
        if (!line.empty()) {
            history_store.push_back(line);
            add_history(line.c_str()); // preload into readline history
        }
    }
    while ((int)history_store.size() > 20)
        history_store.erase(history_store.begin());
}

void save_history() {
    ofstream fout(histfile, ios::trunc);
    int start = max(0, (int)history_store.size() - 20);
    for (int i = start; i < (int)history_store.size(); ++i)
        fout << history_store[i] << "\n";
}

// ---------- Autocomplete logic ----------
static void add_path_matches(const string& prefix, vector<string>& out) {
    const char* p = getenv("PATH");
    if (!p) return;
    string path(p);
    size_t start = 0;
    while (start < path.size()) {
        size_t pos = path.find(':', start);
        string dir = (pos == string::npos) ? path.substr(start)
                                           : path.substr(start, pos - start);
        start = (pos == string::npos) ? path.size() : pos + 1;
        DIR* d = opendir(dir.c_str());
        if (!d) continue;
        struct dirent* e;
        while ((e = readdir(d))) {
            string name = e->d_name;
            if (name.rfind(prefix, 0) == 0) {
                string full = dir + "/" + name;
                struct stat st;
                if (stat(full.c_str(), &st) == 0 && (st.st_mode & S_IXUSR)) {
                    out.push_back(name);
                }
            }
        }
        closedir(d);
    }
}

static vector<string> get_matches(const string& token) {
    vector<string> matches;

    // Builtins
    vector<string> builtins = {
        "cd", "pwd", "echo", "ls", "pinfo",
        "search", "history", "exit", "exitall"
    };
    for (auto& b : builtins)
        if (b.rfind(token, 0) == 0)
            matches.push_back(b);

    // Current directory entries
    DIR* d = opendir(".");
    if (d) {
        struct dirent* e;
        while ((e = readdir(d))) {
            string n = e->d_name;
            if (n.rfind(token, 0) == 0)
                matches.push_back(n);
        }
        closedir(d);
    }

    // PATH matches
    add_path_matches(token, matches);

    // Deduplicate + sort
    sort(matches.begin(), matches.end());
    matches.erase(unique(matches.begin(), matches.end()), matches.end());

    return matches;
}

// GNU Readline completion generator
static char* my_generator(const char* text, int state) {
    static vector<string> matches;
    static size_t index;

    if (state == 0) {
        string token(text);
        matches = get_matches(token);
        index = 0;
    }

    if (index < matches.size())
        return strdup(matches[index++].c_str());
    else
        return nullptr;
}

static char** my_completion(const char* text, int start, int end) {
    (void)start; (void)end;
    return rl_completion_matches(text, my_generator);
}

// ---------- Input function ----------
string read_input_line() {
    // Ensure readline does not override our Ctrl+C / Ctrl+Z handlers
    rl_catch_signals = 0;
    rl_catch_sigwinch = 0;

    // Hook our completion function
    rl_attempted_completion_function = my_completion;

    // Show colored prompt
    string prompt = get_prompt(true);
    char* input = readline(prompt.c_str());

    if (!input)
        return string("__MYSH_EOF__"); // Ctrl+D pressed (EOF)

    string buf(input);
    free(input);

    if (!buf.empty()) {
        history_store.push_back(buf);
        if ((int)history_store.size() > 20)
            history_store.erase(history_store.begin());
        add_history(buf.c_str());
    }

    return buf;
}
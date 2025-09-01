#include "builtins.h"
#include "jobs.h"
#include "prompt.h"
#include "pinfo.h"
#include "search.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <pwd.h>
#include <grp.h>
#include <iomanip>
#include <cstring>
#include <cerrno>
#include <fcntl.h>
#include <cstdlib>
#include <algorithm>
#include <filesystem>

#if defined(__APPLE__)
extern "C" {
    #include <sys/types.h>
    #include <sys/sysctl.h>
    #include <libproc.h>
    #include <sys/ioctl.h>
}
#endif


using namespace std;

static string home_dir;   // Shell’s home directory (set once)

void builtin_cd(char** args) {
    
    static string oldpwd;     // Previous directory
    char cwd[PATH_MAX];

    // Initialize home_dir when function is first called
    if (home_dir.empty()) {
        if (getcwd(cwd, sizeof(cwd)) != nullptr) {
            home_dir = cwd;
        } else {
            perror("getcwd");
            return;
        }
    }

    // Count arguments
    int argc = 0;
    while (args[argc]) argc++;

    if (argc > 2) {
        cerr << "Invalid arguments for error handling.\n";
        return;
    }

    string target;

    if (argc == 1) {
        // No argument → go to home directory
        target = home_dir;
    } else {
        string arg(args[1]);
        if (arg == ".") {
            target = ".";
        } else if (arg == "..") {
            target = "..";
        } else if (arg == "-") {
            if (oldpwd.empty()) {
                cerr << "cd: OLDPWD not set\n";
                return;
            }
            target = oldpwd;
            cout << target << endl; // mimic shell "cd -" behavior
        } else if (arg == "~") {
            target = home_dir;
        } else {
            target = arg; // normal path
        }
    }

    // Save current directory to oldpwd before changing
    if (getcwd(cwd, sizeof(cwd)) != nullptr) {
        oldpwd = cwd;
    }

    if (chdir(target.c_str()) != 0) {
        perror("cd");
    }
}

void builtin_pwd() {
    char buf[PATH_MAX];
    if (getcwd(buf, sizeof(buf))) {
        std::cout << buf << "\n";
    } else {
        perror("pwd");
    }
}

void builtin_echo(char** args) {
    for (int i = 1; args[i]; i++) {
        std::cout << args[i] << " ";
    }
    std::cout << "\n";
}

static vector<string> history;

void builtin_history(char** args) {
    int n = 10; // default
    if (args[1]) {
        n = atoi(args[1]);
        if (n <= 0) n = 10;
    }
    size_t start = (history.size() > (size_t)n) ? history.size() - n : 0;
    for (size_t i = start; i < history.size(); i++) {
        cout << history[i] << "\n";
    }
}

void add_to_history(const string& cmd) {
    history.push_back(cmd);
}


static void print_long_format(const string& path, const string& name) {
    struct stat st;
    string fullpath = path + "/" + name;

    if (stat(fullpath.c_str(), &st) == -1) {
        perror("stat");
        return;
    }

    // File type and permissions
    cout << (S_ISDIR(st.st_mode) ? 'd' : '-');
    cout << ((st.st_mode & S_IRUSR) ? 'r' : '-');
    cout << ((st.st_mode & S_IWUSR) ? 'w' : '-');
    cout << ((st.st_mode & S_IXUSR) ? 'x' : '-');
    cout << ((st.st_mode & S_IRGRP) ? 'r' : '-');
    cout << ((st.st_mode & S_IWGRP) ? 'w' : '-');
    cout << ((st.st_mode & S_IXGRP) ? 'x' : '-');
    cout << ((st.st_mode & S_IROTH) ? 'r' : '-');
    cout << ((st.st_mode & S_IWOTH) ? 'w' : '-');
    cout << ((st.st_mode & S_IXOTH) ? 'x' : '-');

    // Links
    cout << " " << st.st_nlink;

    // Owner and group
    struct passwd* pw = getpwuid(st.st_uid);
    struct group* gr = getgrgid(st.st_gid);
    cout << " " << (pw ? pw->pw_name : to_string(st.st_uid));
    cout << " " << (gr ? gr->gr_name : to_string(st.st_gid));

    // Size
    cout << " " << st.st_size;

    // Last modified time
    char timebuf[80];
    strftime(timebuf, sizeof(timebuf), "%b %d %H:%M", localtime(&st.st_mtime));
    cout << " " << timebuf;

    // Name
    cout << " " << name << endl;
}

static void ls_directory(const string& path, bool flag_a, bool flag_l) {
    DIR* dir = opendir(path.c_str());
    if (!dir) {
        perror("opendir");
        return;
    }

    vector<string> entries;
    struct dirent* entry;
    long total_blocks = 0;

    while ((entry = readdir(dir)) != NULL) {
        string name = entry->d_name;
        if (!flag_a && name[0] == '.') continue; // skip hidden unless -a
        entries.push_back(name);

        if (flag_l) {
            struct stat st;
            string fullpath = path + "/" + name;
            if (stat(fullpath.c_str(), &st) == 0) {
                total_blocks += st.st_blocks;
            }
        }
    }
    closedir(dir);

    sort(entries.begin(), entries.end());

    if (flag_l) {
    #ifdef __APPLE__
    cout << "total " << total_blocks << endl;
    #else
    cout << "total " << total_blocks / 2 << endl;
    #endif
    for (auto& name : entries) {
        print_long_format(path, name);
    }
} else {
    // --- Tabular output (like real ls) ---
    size_t max_len = 0;
    for (auto& n : entries) {
        max_len = max(max_len, n.size());
    }

    // Detect terminal width
    struct winsize w;
    int term_width = 80; // fallback
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == 0) {
        term_width = w.ws_col;
    }

    size_t col_width = max_len + 2;
    size_t cols = term_width / col_width;
    if (cols == 0) cols = 1;

    size_t rows = (entries.size() + cols - 1) / cols;

    // Print row-major
    for (size_t r = 0; r < rows; r++) {
        for (size_t c = 0; c < cols; c++) {
            size_t idx = c * rows + r;
            if (idx < entries.size()) {
                cout << left << setw((int)col_width) << entries[idx];
            }
        }
        cout << endl;
    }
}
}

void builtin_search(char** args) {
    if (args[1] == nullptr) {
        std::cerr << "Error: No filename provided for search\n";
        return;
    }
    
    bool found = search_file(args[1]);
    std::cout << (found ? "true" : "false") << std::endl;
}

void builtin_pinfo(char** args) {
    pid_t pid;
    if (args[1] == nullptr) {
        pid = 0;  // Will use current process
    } else {
        pid = atoi(args[1]);
    }
    pinfo(pid);
}

void builtin_ls(char** args) {
    char cwd[PATH_MAX];
    if (home_dir.empty()) {
        if (getcwd(cwd, sizeof(cwd)) != nullptr) {
            home_dir = cwd;
        } else {
            perror("getcwd");
            return;
        }
    }
    bool flag_a = false, flag_l = false;
    vector<string> targets;

    for (int i = 1; args[i]; i++) {
        string arg = args[i];
        if (arg[0] == '-') {
            for (char c : arg.substr(1)) {
                if (c == 'a') flag_a = true;
                else if (c == 'l') flag_l = true;
            }
        } else {
            targets.push_back(arg);
        }
    }

    if (targets.empty()) {
        targets.push_back(".");
    }

    for (auto& t : targets) {
        string path = t;
        if (t == "~") path = home_dir;

        ls_directory(path, flag_a, flag_l);
    }
}
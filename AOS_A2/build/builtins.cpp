#include "builtins.h"
#include <unistd.h>
#include <iostream>
#include <string.h>
#include <limits.h>

using namespace std;

void builtin_cd(char** args) {
    static string home_dir;   // Shell’s home directory (set once)
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
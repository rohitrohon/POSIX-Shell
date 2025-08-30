#include "builtins.h"
#include <unistd.h>
#include <iostream>
#include <string.h>
#include <limits.h>

void builtin_cd(char** args) {
    if (!args[1]) {
        std::cerr << "cd: missing operand\n";
        return;
    }
    if (chdir(args[1]) != 0) perror("cd");
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
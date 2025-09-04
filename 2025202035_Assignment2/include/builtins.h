
#ifndef BUILTINS_H
#define BUILTINS_H
#include <string>
#include <vector>
bool is_builtin(const std::string& cmd);
int builtin_cd(char** args);
int builtin_pwd(char** args);
int builtin_echo(char** args);
int builtin_ls(char** args);
int builtin_pinfo(char** args);
int builtin_search(char** args);
int builtin_history(char** args);
int builtin_dispatch(char** argv);
#endif

#ifndef BUILTINS_H
#define BUILTINS_H
void builtin_cd(char** args);
void builtin_pwd();
void builtin_echo(char** args);
void builtin_ls(char** args);
void builtin_history(char** args);
void builtin_search(char** args);
#endif
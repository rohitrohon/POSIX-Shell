
#include "prompt.h"
#include "parser.h"
#include "exec.h"
#include "signals.h"
#include "arrow.h"
#include "common.h"
#include <unistd.h>
#include <limits.h>
#include <iostream>
using namespace std;

int main(){
    char cwd[PATH_MAX]; 
    getcwd(cwd,sizeof(cwd)); 
    SHELL_HOME = cwd;
    install_shell_signal_handlers();
    load_history();
    while (true){
        string prompt = get_prompt(false); // get current prompt string from prompt.cpp
        string line = read_input_line(); // read input line with arrow key support from arrow.cpp
        if (line == "__MYSH_EOF__"){ cout << "Process Terminated\n"; break; } //handles Cntl+D
        if (line.empty()) continue;
        string t = trim(line);
        if (t=="exit"){ 
            break; 
        }
        if (t=="exitall"){ 
            cout<<"Exitall: terminating\n"; break; 
        }
        auto cmds = parse_line_strtok(line);
        for (auto &p: cmds)
        { 
            run_parsed(p); //run each parsed command from exec.cpp
            free_parsed(p); 
        }
    }
    save_history();
    return 0;
}

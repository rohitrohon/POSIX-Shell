#include "prompt.h"
#include "parser.h"
#include "exec.h"
#include "builtins.h"
#include "pinfo.h"
#include "history.h"
#include "jobs.h"
#include "signals.h"
#include "utils.h"

#include <iostream>
#include <string>
#include <vector>
#include <unistd.h>
#include <termios.h>
#include <string.h>
#include <stdlib.h>

using namespace std;

string g_home;   // global home directory

// Handle launching in new terminal (macOS / Linux)
void try_new_terminal(int argc, char* argv[]) {
    if (argc == 1) {
        char cwd[1024];
        getcwd(cwd, sizeof(cwd));
        string argv0 = argv[0];

#if defined(__APPLE__)
        // macOS: open new Terminal window and cd into the launch directory
        string cmd = "osascript -e 'tell application \"Terminal\" "
                     "to do script \"cd " + string(cwd) + " && exec " + string(cwd) + "/" + argv0 + " --child\"'";
        system(cmd.c_str());
        exit(0);
#elif defined(__linux__)
        // Linux: open new xterm window, cd into launch directory, then run shell
        string cmd = "xterm -e \"cd " + string(cwd) + " && exec " + string(cwd) + "/" + argv0 + " --child\" &";
        system(cmd.c_str());
        exit(0);
#endif
    }
}

int main(int argc, char* argv[]) {
    // Detect and spawn new terminal if needed
    try_new_terminal(argc, argv);

    // Initialize home directory
    char cwd[1024];
    getcwd(cwd, sizeof(cwd));
    g_home = cwd;

    // Initialize history + signal handlers
    load_history();
    init_signal_handlers();

    while (true) {
        print_prompt();

        string line;
        if (!getline(cin, line)) break;  // Ctrl+D exits
        if (line.empty()) continue;

        add_history(line);

        // Split by semicolons into separate commands
        vector<string> commands = split_semicolons(line);

        for (auto& cmd : commands) {
            // Split by pipes
            vector<string> stages;
            char* cstr = strdup(cmd.c_str());
            char* part = strtok(cstr, "|");
            while (part) {
                stages.push_back(part);
                part = strtok(nullptr, "|"); //for getting the next token
            }
            free(cstr);

            // Parse each stage
            vector<Parsed> parsed_stages;
            for (auto& st : stages) {
                parsed_stages.push_back(tokenize_cmd(st));
            }
            if (!parsed_stages.empty()) {
            bool background = parsed_stages.back().background;


            // Identify the command name
            string cmd_name = parsed_stages[0].argv[0] ? parsed_stages[0].argv[0] : "";

            // Builtin commands present in builtins.cpp
            if (cmd_name == "cd") builtin_cd(parsed_stages[0].argv.data());
            else if (cmd_name == "pwd") builtin_pwd();
            else if (cmd_name == "echo") builtin_echo(parsed_stages[0].argv.data());
            else if (cmd_name == "pinfo") {
                pid_t pid = parsed_stages[0].argv[1] ? stoi(parsed_stages[0].argv[1]) : 0;
                pinfo(pid);
            } //show_history in history.cpp
            else if (cmd_name == "history") {
                int n = 10;
                if (parsed_stages[0].argv[1]) 
                    n = stoi(parsed_stages[0].argv[1]);
                show_history(n);
            }
            // Job control builtins in jobs.cpp
            else if (cmd_name == "jobs") {
                bool verbose = false;
                if (parsed_stages[0].argv[1] && string(parsed_stages[0].argv[1]) == "-v") {
                    verbose = true;
                }
                list_jobs(verbose);
            }
            else if (cmd_name == "fg" && parsed_stages[0].argv[1]) {
                if (parsed_stages[0].argv[1])
                    fg(stoi(parsed_stages[0].argv[1]));
            }
             else if (cmd_name == "bg" && parsed_stages[0].argv[1]) {
                if (parsed_stages[0].argv[1])
                    bg(stoi(parsed_stages[0].argv[1]));
            }
            else if (cmd_name == "sig" && parsed_stages[0].argv[1] && parsed_stages[0].argv[2]) {
                if (parsed_stages[0].argv[1] && parsed_stages[0].argv[2])
                    send_sig(stoi(parsed_stages[0].argv[1]), stoi(parsed_stages[0].argv[2]));
            }
            // Exit
            else if (cmd_name == "exit" || cmd_name == "quit") {
                kill_all_jobs(); //kill_all_jobs in jobs.cpp
                save_history(); //save_history in history.cpp
                return 0;
            }
            else if (cmd_name == "exitall") {
                kill_all_jobs_and_close(); // kill_all_jobs_and_close in jobs.cpp
                save_history();
                return 0;
            }
            // External command
            else {
                run_pipeline(parsed_stages, background); // run_pipeline in exec.cpp
            }
        }

            // Free parsed memory
            for (auto& ps : parsed_stages) {
                free_parsed(ps);
            }
        }
    }

    save_history();
    return 0;
}

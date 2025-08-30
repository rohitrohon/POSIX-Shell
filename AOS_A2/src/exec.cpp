#include "exec.h"
#include "utils.h"
#include "signals.h"
#include "jobs.h"
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <iostream>
#include <vector>
#include <string>
#include <cstring>
#include <errno.h>
#include <array>

using namespace std;

#if defined(__APPLE__)
#include <crt_externs.h>
#define environ (*_NSGetEnviron())
#else
extern char **environ;
#endif

string resolve_path(const string& cmd) {
    if (cmd.find('/') != string::npos) {
        struct stat sb;
        if (stat(cmd.c_str(), &sb) == 0 && (sb.st_mode & S_IXUSR))
            return cmd;
        return "";
    }
    const char* path_env = getenv("PATH");
    if (!path_env) return "";
    string path_var(path_env);
    size_t start = 0, end;
    while ((end = path_var.find(':', start)) != string::npos) {
        string dir = path_var.substr(start, end - start);
        string full = dir + "/" + cmd;
        struct stat sb;
        if (stat(full.c_str(), &sb) == 0 && (sb.st_mode & S_IXUSR))
            return full;
        start = end + 1;
    }
    string dir = path_var.substr(start);
    string full = dir + "/" + cmd;
    struct stat sb;
    if (stat(full.c_str(), &sb) == 0 && (sb.st_mode & S_IXUSR))
        return full;
    return "";
}

void run_pipeline(vector<Parsed>& cmds, bool background) {
    int n = cmds.size();
    vector<pid_t> pids;
    vector<array<int,2>> pipes(n - 1);

    for (int i = 0; i < n - 1; i++) {
        if (pipe(pipes[i].data()) < 0) {
            perror("pipe");
            return;
        }
    }

    for (int i = 0; i < n; i++) {
        pid_t pid = fork();
        if (pid < 0) {
            perror("fork");
            return;
        } else if (pid == 0) {
            // Input redirection
            if (!cmds[i].infile.empty()) {
                int fd = open(cmds[i].infile.c_str(), O_RDONLY);
                if (fd < 0) { perror("open input"); exit(1); }
                dup2(fd, STDIN_FILENO);
                close(fd);
            }

            // Output redirection
            if (!cmds[i].outfile.empty()) {
                int flags = O_WRONLY | O_CREAT | (cmds[i].append ? O_APPEND : O_TRUNC);
                int fd = open(cmds[i].outfile.c_str(), flags, 0644);
                if (fd < 0) { perror("open output"); exit(1); }
                dup2(fd, STDOUT_FILENO);
                close(fd);
            }

            // Pipe redirection
            if (i > 0) dup2(pipes[i-1][0], STDIN_FILENO);
            if (i < n - 1) dup2(pipes[i][1], STDOUT_FILENO);

            // Close all pipes in child
            for (int j = 0; j < n - 1; j++) {
                close(pipes[j][0]);
                close(pipes[j][1]);
            }

            execvp(cmds[i].argv[0], cmds[i].argv.data());
            perror("execvp");
            exit(1);
        } else {
            pids.push_back(pid);
        }
    }

    // Close all pipes in parent
    for (int i = 0; i < n - 1; i++) {
        close(pipes[i][0]);
        close(pipes[i][1]);
    }

    if (background) {
        string cmd_str;
        for (auto& c : cmds) {
            for (int k = 0; c.argv[k]; k++)
                cmd_str += string(c.argv[k]) + " ";
            if (&c != &cmds.back()) cmd_str += "| ";
        }
        add_job(pids[0], cmd_str, true);
        cout << "Started job in background with PID " << pids[0] << endl;
    } else {
        // Wait for all foreground processes
        for (pid_t pid : pids) {
            int status;
            waitpid(pid, &status, 0);
        }
    }
}
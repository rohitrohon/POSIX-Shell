#include "exec.h"
#include "builtins.h"
#include "signals.h"
#include "common.h"
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <cerrno>
#include <cstring>
#include <iostream>
using namespace std;
string SHELL_HOME; // set in main()

string build_cmd_string(const Parsed& p) {
    string s;
    for (size_t i=0; i<p.stages.size(); ++i) {
        auto& st = p.stages[i];
        for (size_t j=0; j+1<st.argv.size(); ++j) {
            if (st.argv[j]) {
                if (!s.empty() && s.back()!=' ') s.push_back(' ');
                s += st.argv[j];
            }
        }
        if (i+1<p.stages.size()) s += " |";
        if (!st.infile.empty()){ s += " < "; s += st.infile; }
        if (!st.outfile.empty()){ s += st.append? " >> " : " > "; s += st.outfile; }
    }
    if (p.background) s += " &";
    return s;
}

static void apply_redirs(const CmdStage& st) {
    if (!st.infile.empty()) {
        int fd = open(st.infile.c_str(), O_RDONLY);
        if (fd<0){ perror(("open < "+st.infile).c_str()); _exit(1); }
        if (dup2(fd, STDIN_FILENO)<0){ perror("dup2 <"); _exit(1); }
        close(fd);
    }
    if (!st.outfile.empty()) {
        int flags = O_WRONLY | O_CREAT | (st.append? O_APPEND : O_TRUNC);
        int fd = open(st.outfile.c_str(), flags, 0644);
        if (fd<0){ perror(("open > "+st.outfile).c_str()); _exit(1); }
        if (dup2(fd, STDOUT_FILENO)<0){ perror("dup2 >"); _exit(1); }
        close(fd);
    }
}

void run_parsed(Parsed& p) {
    int n = (int)p.stages.size();

    // --- Case 1: Single builtin command, no pipe ---
    if (n == 1 && p.stages[0].argv.size()>0 && p.stages[0].argv[0]) {
        string cmd = p.stages[0].argv[0];
        if (is_builtin(cmd)) {
            // run directly in parent
            builtin_dispatch(p.stages[0].argv.data());
            return;
        }
    }

    // --- Case 2: Pipeline or external command(s) ---
    vector<int> fds(2*max(0,n-1), -1);
    for (int i=0; i<n-1; ++i) {
        if (pipe(&fds[2*i])<0){ perror("pipe"); return; }
    }

    pid_t pgid = 0;
    for (int i=0; i<n; ++i) {
        pid_t pid = fork();
        if (pid<0){ perror("fork"); return; }
        if (pid==0) {
            if (pgid==0) pgid = getpid();
            setpgid(0, pgid);

            if (i>0) dup2(fds[2*(i-1)], STDIN_FILENO);
            if (i<n-1) dup2(fds[2*i+1], STDOUT_FILENO);

            for (int fd: fds) if (fd!=-1) close(fd);

            apply_redirs(p.stages[i]);

            // Run builtin inside child (useful for pipelines)
            if (is_builtin(p.stages[i].argv[0])) {
                builtin_dispatch(p.stages[i].argv.data());
                _exit(0);
            } else {
                execvp(p.stages[i].argv[0], p.stages[i].argv.data());
                perror(p.stages[i].argv[0]);
                _exit(127);
            }
        } else {
            if (pgid==0) pgid = pid;
            setpgid(pid, pgid);
        }
    }

    for (int fd: fds) if (fd!=-1) close(fd);

    if (p.background) {
        cout << "[bg] " << pgid << "\n";
        return;
    }

    FG_PGID = pgid;
    int status;
    while (true) {
        pid_t w = waitpid(-pgid, &status, WUNTRACED);
        if (w==-1) {
            if (errno==ECHILD) break;
            if (errno==EINTR) continue;
            break;
        }
        if (WIFSTOPPED(status)) break;
    }
    FG_PGID = 0;
}
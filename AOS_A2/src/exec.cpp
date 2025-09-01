#include "exec.h"
#include "jobs.h"
#include "redir.h"

#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <sys/types.h>
#include <signal.h>
#include <termios.h>
#include <sys/stat.h>

#include <iostream>
#include <vector>
#include <string>
#include <cstring>
#include <errno.h>

using namespace std;

// Helper: join parsed stages into a single readable command string
static string build_cmd_string(const vector<Parsed>& cmds) {
    string out;
    for (size_t i = 0; i < cmds.size(); ++i) {
        const auto &p = cmds[i];
        for (size_t k = 0; k + 1 < p.argv.size(); ++k) { // argv is null-terminated; last is nullptr
            if (p.argv[k]) {
                if (!out.empty() && out.back() != ' ') {}
                out += string(p.argv[k]);
                out += ' ';
            }
        }
        if (i + 1 < cmds.size()) {
            out += "| ";
        }
    }
    return out;
}

void run_pipeline(vector<Parsed>& cmds, bool background) {
    if (cmds.empty()) return;

    int n = (int)cmds.size();

    // Build command string (for jobs / display)
    string cmd_str = build_cmd_string(cmds);

    // Create pipes: for n stages we need n-1 pipes
    vector<int> pipefds; // 2*(n-1) elements stored as pairs [read,write]
    if (n > 1) {
        pipefds.resize(2 * (n - 1));
        for (int i = 0; i < n - 1; ++i) {
            if (pipe(&pipefds[2*i]) < 0) {
                perror("pipe");
                return;
            }
        }
    }

    // container of child pids
    vector<pid_t> child_pids;
    pid_t pgid = 0; // process group id for the pipeline (set to first child's pid)

    // Launch each stage
    for (int i = 0; i < n; ++i) {
        pid_t pid = fork();
        if (pid < 0) {
            perror("fork");
            // cleanup pipes
            for (int fd : pipefds) if (fd > 0) close(fd);
            return;
        }

        if (pid == 0) {
            // === CHILD ===

            // Set process group: each child joins the pipeline group.
            // In child, setpgid(0, pgid==0 ? 0 : pgid) is problematic since parent sets pgid.
            // Simpler: in child set its pgid to its own pid; parent will set others to same group.
            setpgid(0, 0);

            // If not first stage: read from previous pipe
            if (i > 0) {
                int read_fd = pipefds[2*(i-1)];
                if (dup2(read_fd, STDIN_FILENO) < 0) {
                    perror("dup2 stdin");
                    _exit(1);
                }
            }

            // If not last stage: write to next pipe
            if (i < n - 1) {
                int write_fd = pipefds[2*i + 1];
                if (dup2(write_fd, STDOUT_FILENO) < 0) {
                    perror("dup2 stdout");
                    _exit(1);
                }
            }

            // Close all pipe fds in child (they were duplicated)
            for (int fd : pipefds) {
                if (fd > 0) close(fd);
            }

            // Handle input redirection for this stage
            if (!cmds[i].infile.empty()) {
                int in_fd = open(cmds[i].infile.c_str(), O_RDONLY);
                if (in_fd < 0) {
                    std::cerr << "Error: Cannot open input file '" << cmds[i].infile << "': " 
                             << strerror(errno) << std::endl;
                    _exit(1);
                }
                if (dup2(in_fd, STDIN_FILENO) < 0) {
                    std::cerr << "Error: Failed to redirect input" << std::endl;
                    close(in_fd);
                    _exit(1);
                }
                close(in_fd);
            }

            // Handle output redirection for this stage
            if (!cmds[i].outfile.empty()) {
                mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH; // 0644 permissions
                int flags = O_WRONLY | O_CREAT | (cmds[i].append ? O_APPEND : O_TRUNC);
                int out_fd = open(cmds[i].outfile.c_str(), flags, mode);
                if (out_fd < 0) {
                    std::cerr << "Error: Cannot open output file '" << cmds[i].outfile << "': " 
                             << strerror(errno) << std::endl;
                    _exit(1);
                }
                if (dup2(out_fd, STDOUT_FILENO) < 0) {
                    std::cerr << "Error: Failed to redirect output" << std::endl;
                    close(out_fd);
                    _exit(1);
                }
                close(out_fd);
            }

            // Execute external program for this stage
            // argv is char*[], null-terminated (per parser)
            if (cmds[i].argv.empty() || cmds[i].argv[0] == nullptr) {
                fprintf(stderr, "empty command\n");
                _exit(1);
            }

            execvp(cmds[i].argv[0], cmds[i].argv.data());
            // If execvp returns, it's an error
            fprintf(stderr, "%s: command not found\n", cmds[i].argv[0]);
            _exit(127);
        }

        // === PARENT ===

        // First child sets the baseline pgid
        if (pgid == 0) {
            pgid = pid;
        }

        // Ensure the child is in the same process group
        // setpgid in parent to avoid race: set child's pgid to pgid
        if (setpgid(pid, pgid) < 0) {
            // On some systems this may fail if child already called setpgid; ignore EACCES or ESRCH
            // perror("setpgid"); // optional debug
        }

        child_pids.push_back(pid);
    }

    // Parent: close all pipe ends
    for (int fd : pipefds) {
        if (fd > 0) close(fd);
    }

    // If background: just record job and return to prompt
    if (background) {
        // Add job with pgid (so future signals can target group)
        add_job(pgid, cmd_str, true);
        cout << "[" << pgid << "]" << " " << "Started in background\n";
        return;
    }

    // FOREGROUND: give terminal to child process group, wait for it
    // Save shell's PGID
    pid_t shell_pgid = getpgrp();
    // Give terminal to child group
    if (tcsetpgrp(STDIN_FILENO, pgid) < 0) {
        // perror("tcsetpgrp");
    }

    // Wait for the process group to finish or stop
    int status;
    pid_t wpid;
    bool all_exited = false;

    while (!all_exited) {
        all_exited = true;  // Assume all processes have exited until proven otherwise

        for (pid_t child_pid : child_pids) {
            wpid = waitpid(child_pid, &status, WUNTRACED | WNOHANG);
            
            if (wpid == -1) {
                if (errno != ECHILD) {  // Ignore if child has already exited
                    perror("waitpid");
                }
                continue;
            }
            
            if (wpid == 0) {  // Child still running
                all_exited = false;
                continue;
            }

            if (WIFSTOPPED(status)) {
                // Job has been stopped (Ctrl+Z): add to jobs as stopped
                string cmdstr = cmd_str;
                add_job(pgid, cmdstr, false); // running=false -> stopped
                all_exited = false;
                break;
            }

            if (WIFEXITED(status)) {
                if (WEXITSTATUS(status) != 0) {
                    // Command failed, but we don't exit the shell
                    std::cerr << "Command exited with status " << WEXITSTATUS(status) << std::endl;
                }
            } else if (WIFSIGNALED(status)) {
                std::cerr << "Command terminated by signal " << WTERMSIG(status) << std::endl;
            }
        }

        if (!all_exited) {
            // Give a small delay before checking again to reduce CPU usage
            usleep(10000);  // 10ms delay
        }
    }

    // Restore terminal control to shell
    if (tcsetpgrp(STDIN_FILENO, shell_pgid) < 0) {
        // perror("tcsetpgrp restore");
    }

    // Optionally reap remaining children in that group (non-blocking)
    while ((wpid = waitpid(-pgid, &status, WNOHANG)) > 0) { /*nada*/ }
}
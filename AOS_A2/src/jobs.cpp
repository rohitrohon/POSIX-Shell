#include "jobs.h"
#include <signal.h>
#include <unistd.h>
#include <iostream>
#include <cstdlib>   // for system()
#include <sys/wait.h>
#include <termios.h>

using namespace std;

vector<Job> jobs;
int next_job_id = 1;

void add_job(pid_t pid, const string& cmd, bool running, bool stopped) {
    Job j;
    j.job_id = next_job_id++;
    j.pid = pid;
    j.cmd = cmd;       
    j.running = running;
    j.stopped = stopped;
    jobs.push_back(j);
}

void remove_job(pid_t pid) {
    jobs.erase(remove_if(jobs.begin(), jobs.end(),
        [pid](const Job& j){ return j.pid == pid; }), jobs.end());
}

void refresh_jobs() {
    for (auto& j : jobs) {
        int status;
        pid_t result = waitpid(j.pid, &status, WNOHANG | WUNTRACED | WCONTINUED);

        if (result > 0) {
            if (WIFEXITED(status) || WIFSIGNALED(status)) {
                j.running = false;
                j.stopped = false;
            } else if (WIFSTOPPED(status)) {
                j.running = false;
                j.stopped = true;
            } else if (WIFCONTINUED(status)) {
                j.running = true;
                j.stopped = false;
            }
        }
    }

    // Remove finished jobs
    jobs.erase(remove_if(jobs.begin(), jobs.end(),
        [](const Job& j){ return !j.running && !j.stopped; }),
        jobs.end());
}

void list_jobs(bool verbose) {
    refresh_jobs();

    const size_t MAX_LEN = 80;

    for (auto& j : jobs) {
        string display_cmd = j.cmd;
        if (!verbose && display_cmd.size() > MAX_LEN) {
            display_cmd = display_cmd.substr(0, MAX_LEN - 3) + "...";
        }

        cout << "[" << j.job_id << "] "
             << (j.running ? "Running " : (j.stopped ? "Stopped " : "Done "))
             << display_cmd << " [" << j.pid << "]" << endl;
    }
}

void fg(int job_id) {
    for (auto& j : jobs) {
        if (j.job_id == job_id) {
            kill(j.pid, SIGCONT);
            int status;
            waitpid(j.pid, &status, 0);
            remove_job(j.pid);
            return;
        }
    }
    cerr << "fg: no such job" << endl;
}

void bg(int job_id) {
    for (auto& j : jobs) {
        if (j.job_id == job_id) {
            kill(j.pid, SIGCONT);
            j.running = true;
            j.stopped = false;
            return;
        }
    }
    cerr << "bg: no such job" << endl;
}

void send_sig(int job_id, int sig) {
    for (auto& j : jobs) {
        if (j.job_id == job_id) {
            kill(j.pid, sig);
            return;
        }
    }
    cerr << "sig: no such job" << endl;
}

void kill_all_jobs() {
    for (auto& j : jobs) {
        kill(j.pid, SIGKILL);
    }
    jobs.clear();
}

void kill_all_jobs_and_close() {
    kill_all_jobs();  // kill background jobs first

#if defined(__APPLE__)
    // Detect terminal type
    const char* term_app = getenv("TERM_PROGRAM");

    if (term_app && string(term_app) == "iTerm.app") {
        // Close window in iTerm2
        system("osascript -e 'tell application \"iTerm2\" to close (current window)'");
    } else {
        // Default: macOS Terminal
        system("osascript -e 'tell application \"Terminal\" to close front window'");
    }

    // Exit this process
    exit(0);

#elif defined(__linux__)
    // On Linux: kill parent terminal emulator
    const char* term = getenv("TERM");

    if (term && string(term).find("xterm") != string::npos) {
        kill(getppid(), SIGKILL);   // closes xterm
    } else if (term && string(term).find("gnome") != string::npos) {
        kill(getppid(), SIGKILL);   // closes gnome-terminal
    } else if (term && string(term).find("konsole") != string::npos) {
        kill(getppid(), SIGKILL);   // closes KDE Konsole
    } else {
        cerr << "exitall: unsupported terminal type, exiting shell only." << endl;
    }

    exit(0);
#endif
}
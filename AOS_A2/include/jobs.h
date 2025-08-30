#ifndef JOBS_H
#define JOBS_H

#include <string>
#include <vector>
#include <sys/types.h>
using namespace std;

struct Job {
    int job_id;
    pid_t pid;
    string cmd;
    bool running;
    bool stopped;
};

extern vector<Job> jobs;

void add_job(pid_t pid, const string& cmd, bool running = true, bool stopped = false);
void remove_job(pid_t pid);
void refresh_jobs();                 // <-- NEW: update status
void list_jobs(bool verbose = false);
void fg(int job_id);
void bg(int job_id);
void send_sig(int job_id, int sig);
void kill_all_jobs();
void kill_all_jobs_and_close();

#endif
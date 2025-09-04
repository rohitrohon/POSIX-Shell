
#include "builtins.h"
#include "common.h"
#include "history.h"
#include <sys/stat.h>
#include <dirent.h>
#include <pwd.h>
#include <grp.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include <iomanip>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <cstring>
#include <limits.h>
#include <cstdlib>
using namespace std;

static vector<string> builtin_list = {"cd","pwd","echo","ls","pinfo","search","history","exit","exitall"};
bool is_builtin(const string& cmd){
    return find(builtin_list.begin(), builtin_list.end(), cmd) != builtin_list.end();
}

static string home_dir;   // set when shell starts
static string oldpwd;     // OLDPWD

int builtin_cd(char** args) {
    char cwd[PATH_MAX];

    // Initialize home_dir once
    if (home_dir.empty()) {
        if (getcwd(cwd, sizeof(cwd)) != nullptr) {
            home_dir = cwd;
        } else {
            perror("getcwd");
            return -1;
        }
    }

    // Count args
    int argc = 0;
    while (args[argc]) argc++;
    if (argc > 2) {
        cerr << "cd: too many arguments\n";
        return -1;
    }

    string target;
    if (argc == 1) {
        // no args → go to home
        target = home_dir;
    } else {
        string arg(args[1]);
        if (arg == "-") {
            if (oldpwd.empty()) {
                cerr << "cd: OLDPWD not set\n";
                return -1;
            }
            target = oldpwd;
            cout << target << endl;   // bash-like behavior
        } else if (arg == "~" || arg == "~/") {
            target = "/"; // special case to go to home
        } else {
            target = arg;  // can be ., .., abs or rel path
        }
    }

    // Save current dir into oldpwd
    if (getcwd(cwd, sizeof(cwd)) != nullptr) {
        oldpwd = cwd;
    }

    // Try changing directory
    if (chdir(target.c_str()) != 0) {
        perror("cd");
        return -1;
    }

    return 0;
}

int builtin_pwd(char** args) {
    char buf[PATH_MAX];
   if (getcwd(buf, sizeof(buf))) {
        std::cout << buf << "\n";
    } else {
        perror("pwd");
    }
    return 0;
}

int builtin_echo(char** args) {
    for (int i = 1; args[i]; i++) {
        std::cout << args[i] << " ";
    }
    std::cout << "\n";
    return 0;
}

// minimal ls with -a -l flags
static void print_mode(mode_t m){
    cout << (S_ISDIR(m)?'d':'-') << ((m&S_IRUSR)?'r':'-') << ((m&S_IWUSR)?'w':'-') << ((m&S_IXUSR)?'x':'-');
    cout << ((m&S_IRGRP)?'r':'-') << ((m&S_IWGRP)?'w':'-') << ((m&S_IXGRP)?'x':'-');
    cout << ((m&S_IROTH)?'r':'-') << ((m&S_IWOTH)?'w':'-') << ((m&S_IXOTH)?'x':'-') << " ";
}

static void ls_long(const string& path, const string& name){
    struct stat st; string full = path.empty()? name : (path + "/" + name);
    if (lstat(full.c_str(), &st) < 0) { perror("lstat"); return; }
    print_mode(st.st_mode); cout << st.st_nlink << " ";
    struct passwd* pw = getpwuid(st.st_uid); struct group* gr = getgrgid(st.st_gid);
    cout << (pw?pw->pw_name:"?") << " " << (gr?gr->gr_name:"?") << " ";
    cout << st.st_size << " ";
    char tbuf[64]; struct tm lt; localtime_r(&st.st_mtime, &lt); strftime(tbuf, sizeof(tbuf), "%b %e %H:%M", &lt);
    cout << tbuf << " " << name << "\n";
}


#ifdef __APPLE__
#include <sys/sysctl.h>
#include <libproc.h>
#endif

int builtin_pinfo(char** args){
    pid_t pid = getpid();
    if (args[1]) pid = (pid_t)atoi(args[1]);

#ifdef __linux__
    std::string status_path = "/proc/" + std::to_string((int)pid) + "/status";
    FILE* f = fopen(status_path.c_str(), "r");
    if (!f) { perror("pinfo"); return 1; }
    char line[512];
    std::string state = "?";
    std::string vm = "?";
    while (fgets(line, sizeof(line), f)){
        if (strncmp(line, "State:", 6) == 0){
            char *p = strchr(line, '\\t');
            if (!p) p = strchr(line, ' ');
            if (p){ ++p; while (*p==' ') ++p; state = std::string(p); state = state.substr(0, state.find_first_of("\\n\\r")); }
        } else if (strncmp(line, "VmSize:", 7) == 0){
            char *p = line + 7; while (*p==' '||*p=='\\t') ++p; vm = std::string(p); vm = vm.substr(0, vm.find_first_of("\\n\\r"));
        }
    }
    fclose(f);
    char exe_path[PATH_MAX] = "?";
    std::string exe_link = "/proc/" + std::to_string((int)pid) + "/exe";
    ssize_t r = readlink(exe_link.c_str(), exe_path, sizeof(exe_path)-1);
    if (r > 0) exe_path[r] = '\\0'; else strncpy(exe_path, "?", sizeof(exe_path));
    cout << "pid -- " << pid << "\n";
    cout << "Process Status -- " << state << "\n";
    cout << "memory -- " << vm << " {Virtual Memory}\n";
    cout << "Executable Path -- " << exe_path << "\n";
    return 0;
#else
    struct kinfo_proc kip; size_t len = sizeof(kip);
    int mib[4] = { CTL_KERN, KERN_PROC, KERN_PROC_PID, (int)pid };
    if (sysctl(mib, 4, &kip, &len, NULL, 0) == -1) { perror("sysctl"); return 1; }
    char pathbuf[PROC_PIDPATHINFO_MAXSIZE] = "?";
    if (proc_pidpath(pid, pathbuf, sizeof(pathbuf)) <= 0) strncpy(pathbuf, "?", sizeof(pathbuf));
    struct proc_taskinfo pti; ssize_t got = proc_pidinfo(pid, PROC_PIDTASKINFO, 0, &pti, sizeof(pti));
    long vmbytes = 0; if (got == (ssize_t)sizeof(pti)) vmbytes = pti.pti_virtual_size;
    cout << "pid -- " << pid << "\n";
    char st = kip.kp_proc.p_stat;
    string sst = "?";
    if (st == SRUN) 
        sst = "R"; 
    else if (st == SSLEEP) 
        sst = "S"; 
    else if (st == SSTOP) 
        sst = "T"; 
    else if (st == SZOMB) 
        sst = "Z";
    pid_t fg_pgid = tcgetpgrp(STDIN_FILENO);  
    pid_t proc_pgid = getpgid(pid);
    if (fg_pgid != -1 && proc_pgid == fg_pgid) {
        sst += "+";
    }
    cout << "Process Status -- " << sst << "\n";
    if (vmbytes) cout << "memory -- " << vmbytes << " {Virtual Memory}\n"; else cout << "memory -- ? {Virtual Memory}\n";
    cout << "Executable Path -- " << pathbuf << "\n";
    return 0;
#endif
}

static bool search_recursive(const std::string& dir, const std::string& target){
    DIR* d = opendir(dir.c_str());
    if (!d) return false;
    struct dirent* ent;
    bool found = false;
    while ((ent = readdir(d))){
        std::string name = ent->d_name;
        if (name=="." || name=="..") continue;
        if (name == target){ found = true; break; }
        if (ent->d_type == DT_DIR){
            std::string sub = dir + "/" + name;
            if (search_recursive(sub, target)) { found = true; break; }
        }
    }
    closedir(d);
    return found;
}

int builtin_search(char** args){
    if (!args[1]){ std::cerr << "search: missing target\n"; return 1; }
    std::string target = args[1];
    char cwd[PATH_MAX]; getcwd(cwd, sizeof(cwd));
    bool ok = search_recursive(cwd, target);
    std::cout << (ok? "True":"False") << "\n";
    return ok?0:1;
}

#include "arrow.h"
int builtin_history(char** args){
    return show_history_builtin(args);
}

static void print_long_format(const string& path, const string& name) {
    struct stat st;
    string fullpath = path + "/" + name;

    if (stat(fullpath.c_str(), &st) == -1) {
        perror("stat");
        return;
    }

    // File type and permissions
    cout << (S_ISDIR(st.st_mode) ? 'd' : '-');
    cout << ((st.st_mode & S_IRUSR) ? 'r' : '-');
    cout << ((st.st_mode & S_IWUSR) ? 'w' : '-');
    cout << ((st.st_mode & S_IXUSR) ? 'x' : '-');
    cout << ((st.st_mode & S_IRGRP) ? 'r' : '-');
    cout << ((st.st_mode & S_IWGRP) ? 'w' : '-');
    cout << ((st.st_mode & S_IXGRP) ? 'x' : '-');
    cout << ((st.st_mode & S_IROTH) ? 'r' : '-');
    cout << ((st.st_mode & S_IWOTH) ? 'w' : '-');
    cout << ((st.st_mode & S_IXOTH) ? 'x' : '-');

    // Links
    cout << " " << st.st_nlink;

    // Owner and group
    struct passwd* pw = getpwuid(st.st_uid);
    struct group* gr = getgrgid(st.st_gid);
    cout << " " << (pw ? pw->pw_name : to_string(st.st_uid));
    cout << " " << (gr ? gr->gr_name : to_string(st.st_gid));

    // Size
    cout << " " << st.st_size;

    // Last modified time
    char timebuf[80];
    strftime(timebuf, sizeof(timebuf), "%b %d %H:%M", localtime(&st.st_mtime));
    cout << " " << timebuf;

    // Name
    cout << " " << name << endl;
}

static void ls_directory(const string& path, bool flag_a, bool flag_l) {
    DIR* dir = opendir(path.c_str());
    if (!dir) {
        perror("opendir");
        return;
    }

    vector<string> entries;
    struct dirent* entry;
    long total_blocks = 0;

    while ((entry = readdir(dir)) != NULL) {
        string name = entry->d_name;
        if (!flag_a && name[0] == '.') continue; // skip hidden unless -a
        entries.push_back(name);

        if (flag_l) {
            struct stat st;
            string fullpath = path + "/" + name;
            if (stat(fullpath.c_str(), &st) == 0) {
                total_blocks += st.st_blocks;
            }
        }
    }
    closedir(dir);

    sort(entries.begin(), entries.end());

    if (flag_l) {
    #ifdef __APPLE__
    cout << "total " << total_blocks << endl;
    #else
    cout << "total " << total_blocks / 2 << endl;
    #endif
    for (auto& name : entries) {
        print_long_format(path, name);
    }
} else {
    // --- Tabular output (like real ls) ---
    size_t max_len = 0;
    for (auto& n : entries) {
        max_len = max(max_len, n.size());
    }

    // Detect terminal width
    struct winsize w;
    int term_width = 80; // fallback
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == 0) {
        term_width = w.ws_col;
    }

    size_t col_width = max_len + 2;
    size_t cols = term_width / col_width;
    if (cols == 0) cols = 1;

    size_t rows = (entries.size() + cols - 1) / cols;

    // Print row-major
    for (size_t r = 0; r < rows; r++) {
        for (size_t c = 0; c < cols; c++) {
            size_t idx = c * rows + r;
            if (idx < entries.size()) {
                cout << left << setw((int)col_width) << entries[idx];
            }
        }
        cout << endl;
    }
}
}

int builtin_ls(char** args) {
    char cwd[PATH_MAX];
    if (home_dir.empty()) {
        if (getcwd(cwd, sizeof(cwd)) != nullptr) {
            home_dir = cwd;
        } else {
            perror("getcwd");
            return -1;
        }
    }
    bool flag_a = false, flag_l = false;
    vector<string> targets;

    for (int i = 1; args[i]; i++) {
        string arg = args[i];
        if (arg[0] == '-') {
            for (char c : arg.substr(1)) {
                if (c == 'a') flag_a = true;
                else if (c == 'l') flag_l = true;
            }
        } else {
            targets.push_back(arg);
        }
    }

    if (targets.empty()) {
        targets.push_back(".");
    }

    for (auto& t : targets) {
        string path = t;
        if (t == "~") path = "/";

        ls_directory(path, flag_a, flag_l);
    }
    return 0;
}


int builtin_dispatch(char** argv) {
    if (!argv || !argv[0]) return -1;
    string cmd = argv[0];

    if (cmd == "cd") return builtin_cd(argv);
    if (cmd == "pwd") return builtin_pwd(argv);
    if (cmd == "echo") return builtin_echo(argv);
    if (cmd == "ls") return builtin_ls(argv);
    if (cmd == "pinfo") return builtin_pinfo(argv);
    if (cmd == "search") return builtin_search(argv);
    if (cmd == "history") return builtin_history(argv);

    return -1; // not a builtin
}
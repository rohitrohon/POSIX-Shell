#include "prompt.h"
#include "utils.h"
#include <iostream>
#include <unistd.h>
#include <pwd.h>
#include <string.h>

using namespace std;

extern string g_home;

// ANSI color codes
#define COLOR_USER   "\033[1;32m"   // bright green
#define COLOR_HOST   "\033[1;34m"   // bright blue
#define COLOR_PATH   "\033[1;36m"   // cyan
#define COLOR_RESET  "\033[0m"

void print_prompt() {
    // Get username
    const char* username = getenv("USER");
    if (!username) {
        struct passwd* pw = getpwuid(getuid());
        if (pw) username = pw->pw_name;
        else username = "unknown";
    }

    // Get hostname (short, not FQDN)
    char hostname[256];
    gethostname(hostname, sizeof(hostname));
    char* dot = strchr(hostname, '.');
    if (dot) *dot = '\0'; // strip domain suffix

    // Get current working directory
    char cwd[1024];
    getcwd(cwd, sizeof(cwd));
    string cwd_str = cwd;

    // Replace home directory with "~"
    if (cwd_str.find(g_home) == 0) {
        cwd_str.replace(0, g_home.size(), "~");
    }

    // Print colorized prompt
    cout << COLOR_USER << username 
         << "_@_" 
         << COLOR_HOST << hostname 
         << ":" 
         << COLOR_PATH << cwd_str 
         << COLOR_RESET << "> ";
    cout.flush();
}
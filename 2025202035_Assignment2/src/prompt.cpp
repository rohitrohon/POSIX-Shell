
#include "prompt.h"
#include "common.h"
#include <unistd.h>
#include <pwd.h>
#include <limits.h>
#include <iostream>
using namespace std;
static string get_user(){
    struct passwd* pw = getpwuid(getuid()); //get user name from user id
    if (pw && pw->pw_name) //to handle segmentation fault in case pw->pw_name is NULL
        return string(pw->pw_name);
    return "user";
}
static string get_host(){
    char buf[256];
    if (gethostname(buf,sizeof(buf))==0) //Returns 0 on success, -1 on error.
        return string(buf); 
    return "host";
}
string get_prompt(bool for_readline=true){
    char cwd[PATH_MAX]; //c string to store current working directory
    getcwd(cwd,sizeof(cwd));
    string user = get_user();
    string host = get_host();
    string path = tildeify(cwd);
    string cu = "\033[1;36m";   // cyan
    string ch = "\033[1;35m";   // magenta
    string cp = "\033[1;32m";   // green
    string cr = "\033[0m";           // reset
    if (for_readline){ //check if input is from terminal
        // wrap color codes in \001...\002 so that readline can ignore them when calculating prompt length
        cu = "\001" + cu + "\002";
        ch = "\001" + ch + "\002";
        cp = "\001" + cp + "\002";
        cr = "\001" + cr + "\002";
    }

    string p;
    p += cu + user + cr;
    p += "@";
    p += ch + host + cr;
    p += ":";
    p += cp + path + cr;
    p += "> ";
    return p;

}

#include "utils.h"
#include <unistd.h>
#include <limits.h>


std::string get_cwd() {
    char buf[PATH_MAX];
    if (getcwd(buf, sizeof(buf))) return std::string(buf);
    return "";
}
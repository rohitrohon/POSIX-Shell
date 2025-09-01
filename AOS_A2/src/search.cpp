#include "search.h"
#include <iostream>
#include <stack>
#include <string>
#include <dirent.h>
#include <sys/stat.h>
#include <cstring>
#include <unistd.h>

bool search_file(const std::string& filename) {
    std::stack<std::string> directories;
    directories.push(".");
    
    while (!directories.empty()) {
        std::string current_dir = directories.top();
        directories.pop();
        
        DIR* dir = opendir(current_dir.c_str());
        if (!dir) {
            continue;
        }
        
        struct dirent* entry;
        while ((entry = readdir(dir))) {
            // Skip . and ..
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
                continue;
            }
            
            std::string full_path = current_dir + "/" + entry->d_name;
            struct stat path_stat;
            
            if (stat(full_path.c_str(), &path_stat) == 0) {
                if (S_ISDIR(path_stat.st_mode)) {
                    // If it's a directory, add it to the stack for later processing
                    directories.push(full_path);
                } else if (entry->d_name == filename) {
                    // Found the file
                    closedir(dir);
                    return true;
                }
            }
        }
        closedir(dir);
    }
    return false;
}

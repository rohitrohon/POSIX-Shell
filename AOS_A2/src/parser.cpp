#include "parser.h"
#include <iostream>
#include <string.h>

using namespace std;

vector<string> split_semicolons(const string& line) {
    vector<string> res;
    char* input = strdup(line.c_str());
    char* token = strtok(input, ";");
    while (token != nullptr) {
        string part(token);
        if (!part.empty()) res.push_back(part);
        token = strtok(nullptr, ";");
    }
    free(input);
    return res;
}

Parsed tokenize_cmd(const string& cmdline) {
    Parsed p;
    p.append = false;
    p.background = false;

    char* input = strdup(cmdline.c_str());
    char* token = strtok(input, " \t");
    while (token != nullptr) {
        if (strcmp(token, "<") == 0) {
            token = strtok(nullptr, " \t");
            if (token) p.infile = token;
        } else if (strcmp(token, ">") == 0) {
            token = strtok(nullptr, " \t");
            if (token) p.outfile = token;
            p.append = false;
        } else if (strcmp(token, ">>") == 0) {
            token = strtok(nullptr, " \t");
            if (token) p.outfile = token;
            p.append = true;
        } else if (strcmp(token, "&") == 0) {
            p.background = true;
        } else {
            p.argv.push_back(strdup(token));
        }
        token = strtok(nullptr, " \t");
    }
    p.argv.push_back(nullptr); // argv must be null-terminated

    free(input);
    return p;
}

void free_parsed(Parsed& p) {
    for (auto arg : p.argv) {
        if (arg) free(arg);
    }
    p.argv.clear();
}
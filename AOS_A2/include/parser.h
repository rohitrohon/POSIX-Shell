#ifndef PARSER_H
#define PARSER_H

#include <string>
#include <vector>

using namespace std;

struct Parsed {
    vector<char*> argv;
    string infile;
    string outfile;
    bool append;
    bool background;
};

vector<string> split_semicolons(const string& line);
Parsed tokenize_cmd(const string& cmdline);
void free_parsed(Parsed& p);

#endif

#ifndef PARSER_H
#define PARSER_H

#include <string>
#include <vector>

struct Parsed {
    std::vector<char*> argv; // null-terminated vector for execvp
    std::string infile;      // input redirection file ("" if none)
    std::string outfile;     // output redirection file ("" if none)
    bool append = false;     // true if >>
    bool background = false; // true if trailing &
};

Parsed tokenize_cmd(const std::string& s);      // parse a single stage string
std::vector<Parsed> parse_pipeline_line(const std::string& line); // split by | and parse
std::vector<std::string> split_semicolons(const std::string& line);

void free_parsed(Parsed &p); // free argv pointers (strdup used)

#endif

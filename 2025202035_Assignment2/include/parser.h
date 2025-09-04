
#ifndef PARSER_H
#define PARSER_H
#include <string>
#include <vector>
struct CmdStage {
    std::vector<char*> argv;
    std::string infile;
    std::string outfile;
    bool append = false;
};
struct Parsed {
    std::vector<CmdStage> stages;
    bool background = false;
};
std::vector<Parsed> parse_line_strtok(const std::string& line);
void free_parsed(Parsed& p);
#endif

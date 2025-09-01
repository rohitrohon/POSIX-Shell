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

    string current_token;
    bool in_quotes = false;
    char quote_char = 0;

    // First pass: handle quotes and build tokens
    vector<string> tokens;
    string token;
    
    for (size_t i = 0; i < cmdline.length(); i++) {
        char c = cmdline[i];
        
        if (c == '"' || c == '\'') {
            if (!in_quotes) {
                in_quotes = true;
                quote_char = c;
            } else if (c == quote_char) {
                in_quotes = false;
                if (!token.empty()) {
                    tokens.push_back(token);
                    token.clear();
                }
            } else {
                token += c;
            }
        } else if (c == ' ' || c == '\t') {
            if (in_quotes) {
                token += c;
            } else if (!token.empty()) {
                tokens.push_back(token);
                token.clear();
            }
        } else {
            token += c;
        }
    }
    
    if (!token.empty()) {
        tokens.push_back(token);
    }

    // Second pass: process tokens for redirection
    for (size_t i = 0; i < tokens.size(); i++) {
        if (tokens[i] == "<") {
            if (i + 1 < tokens.size()) {
                p.infile = tokens[i + 1];
                i++;
            }
        } else if (tokens[i] == ">") {
            if (i + 1 < tokens.size()) {
                p.outfile = tokens[i + 1];
                p.append = false;
                i++;
            }
        } else if (tokens[i] == ">>") {
            if (i + 1 < tokens.size()) {
                p.outfile = tokens[i + 1];
                p.append = true;
                i++;
            }
        } else if (tokens[i] == "&") {
            p.background = true;
        } else {
            p.argv.push_back(strdup(tokens[i].c_str()));
        }
    }

    p.argv.push_back(nullptr); // argv must be null-terminated
    return p;
}

void free_parsed(Parsed& p) {
    for (auto arg : p.argv) {
        if (arg) free(arg);
    }
    p.argv.clear();
}

Parsed parse_command(char* line) {
    Parsed cmd;
    char* token = strtok(line, " \t\n");

    while (token != nullptr) {
        // Input redirection <
        if (strcmp(token, "<") == 0) {
            token = strtok(nullptr, " \t\n");
            if (token) {
                cmd.infile = token;
            } else {
                cerr << "Error: No input file provided for < redirection\n";
            }
        }
        // Output redirection > or >>
        else if (strcmp(token, ">") == 0 || strcmp(token, ">>") == 0) {
            bool append_mode = (strcmp(token, ">>") == 0);
            token = strtok(nullptr, " \t\n");
            if (token) {
                cmd.outfile = token;
                cmd.append = append_mode;
            } else {
                cerr << "Error: No output file provided for redirection\n";
            }
        }
        // Normal arguments
        else {
            cmd.argv.push_back(token);
        }

        token = strtok(nullptr, " \t\n");
    }

    // Add null terminator for execvp compatibility
    cmd.argv.push_back(nullptr);

    return cmd;
}

vector<Parsed> parse_pipeline(char* line) {
    vector<Parsed> cmds;
    char* saveptr;
    char* stage = strtok_r(line, "|", &saveptr);

    while (stage != nullptr) {
        Parsed p = parse_command(stage);
        cmds.push_back(p);
        stage = strtok_r(nullptr, "|", &saveptr);
    }

    return cmds;
}
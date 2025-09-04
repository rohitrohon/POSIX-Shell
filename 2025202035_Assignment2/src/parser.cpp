
#include "parser.h"
#include "common.h"
#include <cstring>
#include <cstdlib>
#include <vector>

static void push_token(std::vector<char*>& argv, const char* tok){
    if (!tok) return;
    size_t len = strlen(tok);
    char* c = (char*)malloc(len+1);
    memcpy(c, tok, len+1);
    argv.push_back(c);
}
static void null_terminate(std::vector<char*>& argv){ argv.push_back(nullptr); }

static CmdStage parse_stage(char* stage_cstr){
    CmdStage st;
    const char* delims = " \t\r\n";
    char* saveptr = nullptr;
    char* tok = strtok_r(stage_cstr, delims, &saveptr);
    while (tok){
        if (strcmp(tok,"<")==0){
            tok = strtok_r(nullptr, delims, &saveptr);
            if (tok) st.infile = tok;
        } else if (strcmp(tok,">>")==0){
            tok = strtok_r(nullptr, delims, &saveptr);
            if (tok){ st.outfile = tok; st.append = true; }
        } else if (strcmp(tok,">")==0){
            tok = strtok_r(nullptr, delims, &saveptr);
            if (tok){ st.outfile = tok; st.append = false; }
        } else {
            push_token(st.argv, tok);
        }
        tok = strtok_r(nullptr, delims, &saveptr);
    }
    null_terminate(st.argv);
    return st;
}

std::vector<Parsed> parse_line_strtok(const std::string& line){
    std::vector<Parsed> out;
    auto parts = split_simple(line, ';');
    for (auto &part : parts){
        std::string s = trim(part);
        if (s.empty()) continue;
        Parsed P;
        // check background
        size_t i = s.find_last_not_of(" \t\r\n");
        if (i != std::string::npos && s[i]=='&'){
            P.background = true;
            s = trim(s.substr(0,i));
        }
        auto stages = split_simple(s,'|');
        for (auto &stg : stages){
            std::string t = trim(stg);
            if (t.empty()) continue;
            std::vector<char> buf(t.begin(), t.end());
            buf.push_back('\0');
            CmdStage cs = parse_stage(buf.data());
            P.stages.push_back(std::move(cs));
        }
        if (!P.stages.empty()) out.push_back(std::move(P));
    }
    return out;
}

void free_parsed(Parsed& p){
    for (auto &st : p.stages){
        for (char* a : st.argv){
            if (a) free(a);
        }
        st.argv.clear();
    }
    p.stages.clear();
}

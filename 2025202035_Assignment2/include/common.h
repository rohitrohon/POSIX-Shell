
#ifndef COMMON_H
#define COMMON_H
#include <string>
#include <vector>
using namespace std;
extern string SHELL_HOME;
extern pid_t FG_PGID;   
string tildeify(const string& p);
string trim(const string& s);
vector<string> split_simple(const std::string& s, char delim);
#endif

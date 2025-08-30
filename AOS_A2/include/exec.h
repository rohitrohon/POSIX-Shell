#ifndef EXEC_H
#define EXEC_H

#include "parser.h"
#include <vector>
using namespace std;

void run_pipeline(vector<Parsed>& cmds, bool background);

#endif
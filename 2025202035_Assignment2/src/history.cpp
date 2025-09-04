
#include "history.h"
#include "arrow.h"
#include <iostream>
using namespace std;
int show_history_builtin(char** args){
    int limit = 10; if (args[1]) limit = max(0, atoi(args[1]));
    const auto& hist = get_history();
    int total = (int)hist.size(); int start = total>limit? total-limit:0;
    for (int i=start;i<total;++i) cout<<hist[i]<<"\n";
    return 0;
}

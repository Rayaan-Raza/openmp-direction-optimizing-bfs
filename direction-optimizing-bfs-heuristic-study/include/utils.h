#ifndef DOBFS_UTILS_H
#define DOBFS_UTILS_H

#include "types.h"
#include <string>

struct CLIArgs {
    std::string graph_path;
    std::string graph_name   = "unknown";
    std::string graph_family = "unknown";
    int         source       = 0;
    bool        directed     = false;
    BfsMode     mode         = BfsMode::Sequential;
    int         threads      = 1;
    double      alpha        = 14.0;
    double      beta         = 24.0;
    std::string csv_path;
    bool        verify       = false;
};

CLIArgs parse_args(int argc, char* argv[]);

void print_usage(const char* prog);

const char* mode_to_string(BfsMode m);

BfsMode string_to_mode(const std::string& s);

#endif

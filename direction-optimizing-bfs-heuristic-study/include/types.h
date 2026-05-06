#ifndef DOBFS_TYPES_H
#define DOBFS_TYPES_H

#include <string>
#include <vector>

enum class BfsMode { Sequential, TopDownOMP, HybridOMP };

struct LevelMetrics {
    int level;
    int frontier_size;
    long long edge_examinations;
    int newly_visited;
    std::string mode_used; // "top_down" or "bottom_up"
};

struct RunMetrics {
    double runtime_seconds       = 0.0;
    double cpu_time_seconds      = 0.0;
    int    total_visited         = 0;
    int    total_levels          = 0;
    long long total_edge_examinations = 0;
    int    switch_to_bu_level    = -1;
    int    switch_back_td_level  = -1;
    int    top_down_levels       = 0;
    int    bottom_up_levels      = 0;
    int    frontier_at_switch    = 0;
    double visited_fraction_at_switch = 0.0;
    std::vector<LevelMetrics> per_level;
};

struct Graph {
    int       n = 0;
    long long m = 0;
    bool      directed = false;

    std::vector<long long> row_ptr;
    std::vector<int>       col_ind;

    std::vector<long long> rev_row_ptr;
    std::vector<int>       rev_col_ind;
};

struct BFSResult {
    std::vector<int> level;
    std::vector<int> parent;
    RunMetrics       metrics;
};

#endif

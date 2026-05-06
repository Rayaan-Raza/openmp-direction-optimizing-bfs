#include "metrics.h"
#include <fstream>
#include <iomanip>
#include <iostream>

void print_run_metrics(const RunMetrics& m) {
    std::cout << "--- Run Metrics ---\n"
              << "  Runtime:              " << std::fixed << std::setprecision(6)
              << m.runtime_seconds << " s\n"
              << "  CPU time:             " << std::fixed << std::setprecision(6)
              << m.cpu_time_seconds << " s\n"
              << "  Visited:              " << m.total_visited << "\n"
              << "  Levels:               " << m.total_levels << "\n"
              << "  Edge examinations:    " << m.total_edge_examinations << "\n"
              << "  Top-down levels:      " << m.top_down_levels << "\n"
              << "  Bottom-up levels:     " << m.bottom_up_levels << "\n";
    if (m.switch_to_bu_level >= 0) {
        std::cout << "  Switch to BU at lvl:  " << m.switch_to_bu_level << "\n"
                  << "  Frontier at switch:   " << m.frontier_at_switch << "\n"
                  << "  Visited frac at sw:   " << std::setprecision(4)
                  << m.visited_fraction_at_switch << "\n";
    }
    if (m.switch_back_td_level >= 0)
        std::cout << "  Switch back TD lvl:   " << m.switch_back_td_level << "\n";
}

void print_per_level_metrics(const RunMetrics& m) {
    std::cout << "--- Per-Level Metrics ---\n"
              << std::setw(6) << "Level"
              << std::setw(12) << "Frontier"
              << std::setw(18) << "EdgeExams"
              << std::setw(12) << "NewVisited"
              << std::setw(12) << "Mode" << "\n";
    for (auto& lm : m.per_level) {
        std::cout << std::setw(6) << lm.level
                  << std::setw(12) << lm.frontier_size
                  << std::setw(18) << lm.edge_examinations
                  << std::setw(12) << lm.newly_visited
                  << std::setw(12) << lm.mode_used << "\n";
    }
}

bool compare_bfs_results(const BFSResult& ref, const BFSResult& candidate) {
    if (ref.level.size() != candidate.level.size()) {
        std::cerr << "FAIL: level array size mismatch ("
                  << ref.level.size() << " vs " << candidate.level.size() << ")\n";
        return false;
    }
    bool pass = true;
    int n = static_cast<int>(ref.level.size());
    for (int i = 0; i < n; i++) {
        if (ref.level[i] != candidate.level[i]) {
            std::cerr << "FAIL: vertex " << i
                      << " level mismatch: ref=" << ref.level[i]
                      << " cand=" << candidate.level[i] << "\n";
            pass = false;
            if (!pass) break; // report first mismatch
        }
    }
    if (pass)
        std::cout << "PASS: level arrays match (" << n << " vertices)\n";
    return pass;
}

static const char* mode_str(BfsMode m) {
    switch (m) {
        case BfsMode::Sequential: return "seq";
        case BfsMode::TopDownOMP: return "topdown_omp";
        case BfsMode::HybridOMP: return "hybrid_omp";
    }
    return "unknown";
}

void write_csv_header(const std::string& path) {
    std::ofstream out(path);
    out << "graph_name,graph_family,directed,source,mode,threads,alpha,beta,"
        << "runtime_seconds,cpu_time_seconds,total_visited,total_levels,total_edge_examinations,"
        << "switch_to_bottom_up_level,switch_back_to_top_down_level,"
        << "top_down_levels,bottom_up_levels,frontier_at_switch,"
        << "visited_fraction_at_switch,speedup,efficiency\n";
}

void write_csv_row(const std::string& path,
                   const std::string& graph_name,
                   const std::string& graph_family,
                   bool directed,
                   int source,
                   BfsMode mode,
                   int threads,
                   double alpha,
                   double beta,
                   const RunMetrics& m,
                   double speedup,
                   double efficiency) {
    std::ofstream out(path, std::ios::app);
    out << graph_name << ","
        << graph_family << ","
        << (directed ? 1 : 0) << ","
        << source << ","
        << mode_str(mode) << ","
        << threads << ","
        << std::fixed << std::setprecision(1) << alpha << ","
        << beta << ","
        << std::setprecision(9) << m.runtime_seconds << ","
        << m.cpu_time_seconds << ","
        << m.total_visited << ","
        << m.total_levels << ","
        << m.total_edge_examinations << ","
        << m.switch_to_bu_level << ","
        << m.switch_back_td_level << ","
        << m.top_down_levels << ","
        << m.bottom_up_levels << ","
        << m.frontier_at_switch << ","
        << std::setprecision(6) << m.visited_fraction_at_switch << ","
        << std::setprecision(4) << speedup << ","
        << efficiency << "\n";
}

void write_per_level_csv(const std::string& path,
                         const std::string& graph_name,
                         int source,
                         BfsMode mode,
                         const RunMetrics& m) {
    std::ofstream out(path);
    out << "graph_name,source,mode,level,frontier_size,edge_examinations,"
        << "newly_visited,mode_used\n";
    for (auto& lm : m.per_level) {
        out << graph_name << ","
            << source << ","
            << mode_str(mode) << ","
            << lm.level << ","
            << lm.frontier_size << ","
            << lm.edge_examinations << ","
            << lm.newly_visited << ","
            << lm.mode_used << "\n";
    }
}

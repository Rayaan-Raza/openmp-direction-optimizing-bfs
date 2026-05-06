#ifndef DOBFS_METRICS_H
#define DOBFS_METRICS_H

#include "types.h"
#include <string>

void print_run_metrics(const RunMetrics& m);

void print_per_level_metrics(const RunMetrics& m);

bool compare_bfs_results(const BFSResult& ref, const BFSResult& candidate);

void write_csv_header(const std::string& path);

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
                   double efficiency);

void write_per_level_csv(const std::string& path,
                         const std::string& graph_name,
                         int source,
                         BfsMode mode,
                         const RunMetrics& m);

#endif

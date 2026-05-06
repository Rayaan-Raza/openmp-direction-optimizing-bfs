#ifndef DOBFS_GRAPH_H
#define DOBFS_GRAPH_H

#include "types.h"
#include <string>

Graph load_edge_list(const std::string& path, bool directed);

void validate_graph(const Graph& g);

void print_graph_stats(const Graph& g, const std::string& name);

#endif

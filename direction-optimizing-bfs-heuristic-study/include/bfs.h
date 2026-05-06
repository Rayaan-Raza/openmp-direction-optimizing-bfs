#ifndef DOBFS_BFS_H
#define DOBFS_BFS_H

#include "types.h"

BFSResult bfs_sequential_topdown(const Graph& g, int source);

BFSResult bfs_topdown_omp(const Graph& g, int source, int num_threads);

BFSResult bfs_hybrid_omp(const Graph& g, int source, int num_threads,
                          double alpha, double beta);

#endif

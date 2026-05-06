#include "bfs.h"
#include <ctime>
#include <omp.h>
#include <queue>

BFSResult bfs_sequential_topdown(const Graph& g, int source) {
    BFSResult result;
    result.level.assign(g.n, -1);
    result.parent.assign(g.n, -1);

    if (source < 0 || source >= g.n)
        return result;

    result.level[source] = 0;
    result.parent[source] = source;

    std::queue<int> frontier;
    frontier.push(source);

    int current_level = 0;
    int visited_count = 1;

    std::clock_t cpu_start = std::clock();
    double t_start = omp_get_wtime();

    while (!frontier.empty()) {
        int frontier_size = static_cast<int>(frontier.size());
        long long edge_exams = 0;
        int newly_visited = 0;

        std::queue<int> next_frontier;

        for (int i = 0; i < frontier_size; i++) {
            int u = frontier.front();
            frontier.pop();

            long long start = g.row_ptr[u];
            long long end   = g.row_ptr[u + 1];
            edge_exams += (end - start);

            for (long long j = start; j < end; j++) {
                int v = g.col_ind[j];
                if (result.level[v] == -1) {
                    result.level[v]  = current_level + 1;
                    result.parent[v] = u;
                    next_frontier.push(v);
                    newly_visited++;
                    visited_count++;
                }
            }
        }

        LevelMetrics lm;
        lm.level             = current_level;
        lm.frontier_size     = frontier_size;
        lm.edge_examinations = edge_exams;
        lm.newly_visited     = newly_visited;
        lm.mode_used         = "top_down";
        result.metrics.per_level.push_back(lm);
        result.metrics.total_edge_examinations += edge_exams;

        frontier = std::move(next_frontier);
        current_level++;
    }

    double t_end = omp_get_wtime();
    std::clock_t cpu_end = std::clock();

    result.metrics.runtime_seconds = t_end - t_start;
    result.metrics.cpu_time_seconds =
        static_cast<double>(cpu_end - cpu_start) / CLOCKS_PER_SEC;
    result.metrics.total_visited   = visited_count;
    result.metrics.total_levels    = current_level;
    result.metrics.top_down_levels = current_level;

    return result;
}

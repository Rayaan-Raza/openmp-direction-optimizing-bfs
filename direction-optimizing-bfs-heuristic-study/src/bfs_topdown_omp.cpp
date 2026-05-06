#include "bfs.h"
#include <ctime>
#include <omp.h>
#include <cstring>
#include <vector>

BFSResult bfs_topdown_omp(const Graph& g, int source, int num_threads) {
    BFSResult result;
    result.level.assign(g.n, -1);
    result.parent.assign(g.n, -1);

    if (source < 0 || source >= g.n)
        return result;

    omp_set_num_threads(num_threads);

    result.level[source]  = 0;
    result.parent[source] = source;

    std::vector<int> frontier;
    frontier.reserve(g.n);
    frontier.push_back(source);

    int current_level = 0;
    int visited_count = 1;

    // per-thread local frontiers
    std::vector<std::vector<int>> local_fronts(num_threads);
    for (auto& lf : local_fronts)
        lf.reserve(g.n / num_threads + 256);

    std::vector<int> prefix(num_threads + 1, 0);

    std::clock_t cpu_start = std::clock();
    double t_start = omp_get_wtime();

    while (!frontier.empty()) {
        int frontier_size = static_cast<int>(frontier.size());
        long long edge_exams = 0;

        // clear local frontiers
        for (auto& lf : local_fronts)
            lf.clear();

        int total_discovered = 0;

        #pragma omp parallel reduction(+:edge_exams, total_discovered)
        {
            int tid = omp_get_thread_num();
            auto& my_next = local_fronts[tid];

            #pragma omp for schedule(dynamic, 64)
            for (int i = 0; i < frontier_size; i++) {
                int u = frontier[i];
                long long start = g.row_ptr[u];
                long long end   = g.row_ptr[u + 1];
                edge_exams += (end - start);

                for (long long j = start; j < end; j++) {
                    int v = g.col_ind[j];
                    if (result.level[v] == -1) {
                        // benign race: multiple threads may write the same
                        // value (current_level+1); level stays correct
                        result.level[v]  = current_level + 1;
                        result.parent[v] = u;
                        my_next.push_back(v);
                        total_discovered++;
                    }
                }
            }
        }

        // prefix-sum merge of local frontiers into global frontier
        prefix[0] = 0;
        for (int t = 0; t < num_threads; t++)
            prefix[t + 1] = prefix[t] + static_cast<int>(local_fronts[t].size());

        int next_size = prefix[num_threads];
        frontier.resize(next_size);

        #pragma omp parallel
        {
            int tid = omp_get_thread_num();
            if (tid < num_threads) {
                std::memcpy(frontier.data() + prefix[tid],
                            local_fronts[tid].data(),
                            local_fronts[tid].size() * sizeof(int));
            }
        }

        // deduplicate: vertices discovered by multiple threads appear multiple
        // times. Keep only the first occurrence for each vertex.
        int write_pos = 0;
        for (int i = 0; i < next_size; i++) {
            int v = frontier[i];
            if (result.level[v] == current_level + 1) {
                // mark as processed to detect duplicates
                result.level[v] = -(current_level + 1) - 2;
                frontier[write_pos++] = v;
            }
        }
        frontier.resize(write_pos);
        // restore correct level values
        for (int i = 0; i < write_pos; i++)
            result.level[frontier[i]] = current_level + 1;

        visited_count += write_pos;

        LevelMetrics lm;
        lm.level             = current_level;
        lm.frontier_size     = frontier_size;
        lm.edge_examinations = edge_exams;
        lm.newly_visited     = write_pos;
        lm.mode_used         = "top_down";
        result.metrics.per_level.push_back(lm);
        result.metrics.total_edge_examinations += edge_exams;

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

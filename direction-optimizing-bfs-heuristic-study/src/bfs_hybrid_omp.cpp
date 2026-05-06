#include "bfs.h"
#include <ctime>
#include <omp.h>
#include <cstring>
#include <vector>

// Top-down step: expand frontier through forward CSR, return newly discovered
// vertices. Updates mf with sum of out-degrees of newly discovered vertices.
static int hybrid_topdown_step(
        const Graph& g,
        std::vector<int>& level,
        std::vector<int>& parent,
        std::vector<int>& frontier,
        int current_level,
        long long& edge_exams,
        long long& mf_new,
        int num_threads,
        std::vector<std::vector<int>>& local_fronts,
        std::vector<int>& prefix) {

    int frontier_size = static_cast<int>(frontier.size());
    edge_exams = 0;
    mf_new = 0;

    for (auto& lf : local_fronts)
        lf.clear();

    #pragma omp parallel reduction(+:edge_exams, mf_new)
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
                if (level[v] == -1) {
                    level[v]  = current_level + 1;
                    parent[v] = u;
                    my_next.push_back(v);
                    mf_new += (g.row_ptr[v + 1] - g.row_ptr[v]);
                }
            }
        }
    }

    // prefix-sum merge
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

    // deduplicate (same technique as bfs_topdown_omp)
    int write_pos = 0;
    for (int i = 0; i < next_size; i++) {
        int v = frontier[i];
        if (level[v] == current_level + 1) {
            level[v] = -(current_level + 1) - 2;
            frontier[write_pos++] = v;
        }
    }
    frontier.resize(write_pos);
    for (int i = 0; i < write_pos; i++)
        level[frontier[i]] = current_level + 1;

    return write_pos;
}

// Bottom-up step: for each unvisited vertex, check if any in-neighbor is in
// frontier. Uses reverse CSR. Returns count of newly discovered vertices.
static int hybrid_bottomup_step(
        const Graph& g,
        std::vector<int>& level,
        std::vector<int>& parent,
        const std::vector<bool>& in_frontier,
        std::vector<int>& frontier,
        int current_level,
        long long& edge_exams,
        long long& mf_new) {

    int n = g.n;
    edge_exams = 0;
    mf_new = 0;
    int discovered = 0;

    // collect newly discovered into a shared vector via thread-local lists
    std::vector<int> new_front;
    new_front.reserve(n / 4);

    #pragma omp parallel
    {
        std::vector<int> my_new;
        long long my_edges = 0;
        long long my_mf = 0;
        int my_disc = 0;

        #pragma omp for schedule(dynamic, 256)
        for (int v = 0; v < n; v++) {
            if (level[v] != -1) continue;

            long long start = g.rev_row_ptr[v];
            long long end   = g.rev_row_ptr[v + 1];

            for (long long j = start; j < end; j++) {
                my_edges++;
                int u = g.rev_col_ind[j];
                if (in_frontier[u]) {
                    level[v]  = current_level + 1;
                    parent[v] = u;
                    my_new.push_back(v);
                    my_disc++;
                    my_mf += (g.row_ptr[v + 1] - g.row_ptr[v]);
                    break; // only need one parent
                }
            }
        }

        #pragma omp critical
        {
            edge_exams += my_edges;
            mf_new     += my_mf;
            discovered += my_disc;
            new_front.insert(new_front.end(), my_new.begin(), my_new.end());
        }
    }

    frontier = std::move(new_front);
    return discovered;
}

BFSResult bfs_hybrid_omp(const Graph& g, int source, int num_threads,
                          double alpha, double beta) {
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

    // mf = edges from frontier, mu = total unexplored edges
    long long mf = g.row_ptr[source + 1] - g.row_ptr[source];
    long long mu = g.m;

    int current_level = 0;
    int visited_count = 1;

    // frontier membership bitmap for bottom-up
    std::vector<bool> in_frontier(g.n, false);
    in_frontier[source] = true;

    // per-thread buffers for top-down
    std::vector<std::vector<int>> local_fronts(num_threads);
    for (auto& lf : local_fronts)
        lf.reserve(g.n / num_threads + 256);
    std::vector<int> prefix(num_threads + 1, 0);

    bool using_bottom_up = false;
    bool first_bu_switch_recorded = false;

    std::clock_t cpu_start = std::clock();
    double t_start = omp_get_wtime();

    while (!frontier.empty()) {
        int frontier_size = static_cast<int>(frontier.size());
        long long edge_exams = 0;
        long long mf_new = 0;
        int newly_visited = 0;
        std::string mode_used;

        bool should_switch_to_bu = (mf > mu / alpha);

        if (!using_bottom_up && should_switch_to_bu) {
            using_bottom_up = true;
            if (!first_bu_switch_recorded) {
                result.metrics.switch_to_bu_level    = current_level;
                result.metrics.frontier_at_switch     = frontier_size;
                result.metrics.visited_fraction_at_switch =
                    static_cast<double>(visited_count) / g.n;
                first_bu_switch_recorded = true;
            }
        }

        if (using_bottom_up) {
            // rebuild frontier bitmap
            std::fill(in_frontier.begin(), in_frontier.end(), false);
            for (int v : frontier) in_frontier[v] = true;

            newly_visited = hybrid_bottomup_step(
                g, result.level, result.parent,
                in_frontier, frontier, current_level,
                edge_exams, mf_new);

            mode_used = "bottom_up";
            result.metrics.bottom_up_levels++;

            // decide whether to switch back to top-down
            int nf = static_cast<int>(frontier.size());
            bool stay_bu = (mf_new > mu / alpha) ||
                           (nf >= frontier_size)  ||
                           (nf > g.n / beta);
            if (!stay_bu) {
                using_bottom_up = false;
                if (result.metrics.switch_back_td_level < 0)
                    result.metrics.switch_back_td_level = current_level + 1;
            }
        } else {
            newly_visited = hybrid_topdown_step(
                g, result.level, result.parent,
                frontier, current_level,
                edge_exams, mf_new, num_threads,
                local_fronts, prefix);

            mode_used = "top_down";
            result.metrics.top_down_levels++;
        }

        mf = mf_new;
        mu -= edge_exams;
        if (mu < 0) mu = 0;
        visited_count += newly_visited;

        LevelMetrics lm;
        lm.level             = current_level;
        lm.frontier_size     = frontier_size;
        lm.edge_examinations = edge_exams;
        lm.newly_visited     = newly_visited;
        lm.mode_used         = mode_used;
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

    return result;
}

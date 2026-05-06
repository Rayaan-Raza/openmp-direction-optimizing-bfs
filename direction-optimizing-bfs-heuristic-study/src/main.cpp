#include "bfs.h"
#include "graph.h"
#include "metrics.h"
#include "utils.h"

#include <fstream>
#include <iostream>
#include <omp.h>

int main(int argc, char* argv[]) {
    CLIArgs args;
    try {
        args = parse_args(argc, argv);
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }

    Graph g;
    try {
        g = load_edge_list(args.graph_path, args.directed);
        validate_graph(g);
    } catch (const std::exception& e) {
        std::cerr << "Graph load error: " << e.what() << "\n";
        return 1;
    }
    print_graph_stats(g, args.graph_name);

    if (args.source < 0 || args.source >= g.n) {
        std::cerr << "Source " << args.source << " out of range [0, "
                  << g.n - 1 << "]\n";
        return 1;
    }

    BFSResult result;
    std::cout << "\nRunning BFS mode: " << mode_to_string(args.mode)
              << " | source=" << args.source
              << " | threads=" << args.threads << "\n";

    switch (args.mode) {
        case BfsMode::Sequential:
            result = bfs_sequential_topdown(g, args.source);
            break;
        case BfsMode::TopDownOMP:
            result = bfs_topdown_omp(g, args.source, args.threads);
            break;
        case BfsMode::HybridOMP:
            std::cout << "  alpha=" << args.alpha
                      << " beta=" << args.beta << "\n";
            result = bfs_hybrid_omp(g, args.source, args.threads,
                                     args.alpha, args.beta);
            break;
    }

    print_run_metrics(result.metrics);
    print_per_level_metrics(result.metrics);

    if (args.verify && args.mode != BfsMode::Sequential) {
        std::cout << "\nVerifying against sequential BFS...\n";
        BFSResult ref = bfs_sequential_topdown(g, args.source);
        compare_bfs_results(ref, result);
    }

    if (!args.csv_path.empty()) {
        // write header if file doesn't exist yet
        {
            std::ifstream probe(args.csv_path);
            if (!probe.good())
                write_csv_header(args.csv_path);
        }
        double seq_time = 0.0;
        if (args.mode != BfsMode::Sequential) {
            BFSResult seq = bfs_sequential_topdown(g, args.source);
            seq_time = seq.metrics.runtime_seconds;
        } else {
            seq_time = result.metrics.runtime_seconds;
        }
        double speedup    = (result.metrics.runtime_seconds > 0)
                                ? seq_time / result.metrics.runtime_seconds
                                : 0.0;
        double efficiency = (args.threads > 0) ? speedup / args.threads : 0.0;

        write_csv_row(args.csv_path, args.graph_name, args.graph_family,
                      args.directed, args.source, args.mode, args.threads,
                      args.alpha, args.beta, result.metrics,
                      speedup, efficiency);
        std::cout << "\nResults appended to " << args.csv_path << "\n";
    }

    return 0;
}

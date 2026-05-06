#include "utils.h"
#include <iostream>
#include <stdexcept>
#include <string>

const char* mode_to_string(BfsMode m) {
    switch (m) {
        case BfsMode::Sequential: return "seq";
        case BfsMode::TopDownOMP: return "topdown_omp";
        case BfsMode::HybridOMP: return "hybrid_omp";
    }
    return "unknown";
}

BfsMode string_to_mode(const std::string& s) {
    if (s == "seq")          return BfsMode::Sequential;
    if (s == "topdown_omp")  return BfsMode::TopDownOMP;
    if (s == "hybrid_omp")   return BfsMode::HybridOMP;
    throw std::runtime_error("Unknown BFS mode: " + s);
}

static std::string get_arg(const std::string& arg, const std::string& prefix) {
    if (arg.substr(0, prefix.size()) == prefix)
        return arg.substr(prefix.size());
    return "";
}

void print_usage(const char* prog) {
    std::cerr
        << "Usage: " << prog << " <graph_path> [options]\n"
        << "Options:\n"
        << "  --source=N        BFS source vertex (default 0)\n"
        << "  --directed=0|1    directed graph semantics (default 0)\n"
        << "  --mode=MODE       seq | topdown_omp | hybrid_omp (default seq)\n"
        << "  --threads=N       OpenMP thread count (default 1)\n"
        << "  --alpha=F         hybrid switch parameter (default 14.0)\n"
        << "  --beta=F          hybrid switch-back parameter (default 24.0)\n"
        << "  --csv=PATH        append CSV results to this file\n"
        << "  --name=STR        graph name for CSV (default: filename)\n"
        << "  --family=STR      graph family for CSV (default: unknown)\n"
        << "  --verify          also run sequential BFS and compare levels\n";
}

CLIArgs parse_args(int argc, char* argv[]) {
    if (argc < 2) {
        print_usage(argv[0]);
        throw std::runtime_error("No graph path provided");
    }

    CLIArgs args;
    args.graph_path = argv[1];

    // derive default name from path
    auto pos = args.graph_path.find_last_of("/\\");
    args.graph_name = (pos != std::string::npos)
                          ? args.graph_path.substr(pos + 1)
                          : args.graph_path;

    for (int i = 2; i < argc; i++) {
        std::string a = argv[i];
        std::string val;

        if (!(val = get_arg(a, "--source=")).empty())
            args.source = std::stoi(val);
        else if (!(val = get_arg(a, "--directed=")).empty())
            args.directed = (val == "1");
        else if (!(val = get_arg(a, "--mode=")).empty())
            args.mode = string_to_mode(val);
        else if (!(val = get_arg(a, "--threads=")).empty())
            args.threads = std::stoi(val);
        else if (!(val = get_arg(a, "--alpha=")).empty())
            args.alpha = std::stod(val);
        else if (!(val = get_arg(a, "--beta=")).empty())
            args.beta = std::stod(val);
        else if (!(val = get_arg(a, "--csv=")).empty())
            args.csv_path = val;
        else if (!(val = get_arg(a, "--name=")).empty())
            args.graph_name = val;
        else if (!(val = get_arg(a, "--family=")).empty())
            args.graph_family = val;
        else if (a == "--verify")
            args.verify = true;
        else {
            std::cerr << "Unknown argument: " << a << "\n";
            print_usage(argv[0]);
            throw std::runtime_error("Unknown argument: " + a);
        }
    }
    return args;
}

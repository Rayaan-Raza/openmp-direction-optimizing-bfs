#include "graph.h"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <utility>

static bool has_suffix(const std::string& s, const std::string& suffix) {
    if (s.size() < suffix.size()) return false;
    return s.compare(s.size() - suffix.size(), suffix.size(), suffix) == 0;
}

static std::vector<std::pair<int,int>> read_edges(const std::string& path) {
    std::ifstream in(path);
    if (!in.is_open())
        throw std::runtime_error("Cannot open graph file: " + path);

    std::vector<std::pair<int,int>> edges;
    std::string line;
    const bool is_mtx = has_suffix(path, ".mtx");
    bool consumed_mtx_size_line = false;

    while (std::getline(in, line)) {
        if (line.empty() || line[0] == '#' || line[0] == '%')
            continue;
        std::istringstream iss(line);
        int u, v;
        if (!(iss >> u >> v))
            continue;

        // Matrix Market format has one "size" line: nrows ncols nnz.
        // Skip it once, then convert all edge endpoints from 1-based to 0-based.
        if (is_mtx && !consumed_mtx_size_line) {
            consumed_mtx_size_line = true;
            continue;
        }
        if (is_mtx) {
            u -= 1;
            v -= 1;
        }
        if (u < 0 || v < 0)
            throw std::runtime_error("Negative vertex ID after parsing: " + line);
        edges.emplace_back(u, v);
    }
    return edges;
}

static void build_csr(const std::vector<std::pair<int,int>>& edges, int n,
                      std::vector<long long>& row_ptr,
                      std::vector<int>& col_ind) {
    row_ptr.assign(n + 1, 0);
    for (auto& [u, v] : edges)
        row_ptr[u + 1]++;
    for (int i = 1; i <= n; i++)
        row_ptr[i] += row_ptr[i - 1];

    col_ind.resize(edges.size());
    std::vector<long long> offset(row_ptr.begin(), row_ptr.end());
    for (auto& [u, v] : edges)
        col_ind[offset[u]++] = v;

    // sort adjacency lists for deterministic behavior
    for (int i = 0; i < n; i++) {
        auto begin = col_ind.begin() + row_ptr[i];
        auto end   = col_ind.begin() + row_ptr[i + 1];
        std::sort(begin, end);
    }
}

Graph load_edge_list(const std::string& path, bool directed) {
    auto raw = read_edges(path);
    if (raw.empty())
        throw std::runtime_error("No edges found in " + path);

    int max_id = 0;
    for (auto& [u, v] : raw)
        max_id = std::max(max_id, std::max(u, v));

    // expand for undirected, remove self-loops
    std::vector<std::pair<int,int>> edges;
    edges.reserve(directed ? raw.size() : raw.size() * 2);
    for (auto& [u, v] : raw) {
        if (u == v) continue; // self-loop
        edges.emplace_back(u, v);
        if (!directed)
            edges.emplace_back(v, u);
    }

    // deduplicate
    std::sort(edges.begin(), edges.end());
    edges.erase(std::unique(edges.begin(), edges.end()), edges.end());

    Graph g;
    g.n = max_id + 1;
    g.directed = directed;

    // forward CSR
    build_csr(edges, g.n, g.row_ptr, g.col_ind);
    g.m = static_cast<long long>(g.col_ind.size());

    if (directed) {
        // build reverse CSR: flip every edge
        std::vector<std::pair<int,int>> rev;
        rev.reserve(edges.size());
        for (auto& [u, v] : edges)
            rev.emplace_back(v, u);
        build_csr(rev, g.n, g.rev_row_ptr, g.rev_col_ind);
    } else {
        // undirected: reverse CSR is identical to forward CSR
        g.rev_row_ptr = g.row_ptr;
        g.rev_col_ind = g.col_ind;
    }

    return g;
}

void validate_graph(const Graph& g) {
    if (g.row_ptr.size() != static_cast<size_t>(g.n + 1))
        throw std::runtime_error("row_ptr size mismatch");
    if (g.row_ptr[g.n] != g.m)
        throw std::runtime_error("row_ptr[n] != m");
    for (int v : g.col_ind) {
        if (v < 0 || v >= g.n)
            throw std::runtime_error("Out-of-range neighbor ID: " + std::to_string(v));
    }
    if (g.directed) {
        if (g.rev_row_ptr[g.n] != g.m)
            throw std::runtime_error("Reverse CSR edge count mismatch");
        for (int v : g.rev_col_ind) {
            if (v < 0 || v >= g.n)
                throw std::runtime_error("Out-of-range reverse neighbor ID: " + std::to_string(v));
        }
    }
}

void print_graph_stats(const Graph& g, const std::string& name) {
    std::cout << "=== Graph: " << name << " ===\n"
              << "  Vertices:  " << g.n << "\n"
              << "  Edges:     " << g.m
              << (g.directed ? " (directed)" : " (undirected, stored both dirs)") << "\n";

    int max_deg = 0, min_deg = g.n;
    long long sum_deg = 0;
    for (int i = 0; i < g.n; i++) {
        int deg = static_cast<int>(g.row_ptr[i + 1] - g.row_ptr[i]);
        max_deg = std::max(max_deg, deg);
        min_deg = std::min(min_deg, deg);
        sum_deg += deg;
    }
    std::cout << "  Avg degree: " << (g.n > 0 ? static_cast<double>(sum_deg) / g.n : 0.0) << "\n"
              << "  Min degree: " << min_deg << "\n"
              << "  Max degree: " << max_deg << "\n";
}

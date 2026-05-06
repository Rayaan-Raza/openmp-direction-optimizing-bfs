# Technical Report: Direction-Optimizing BFS Heuristic Study

## 1) Executive Summary

This project investigates how to make breadth-first search (BFS) faster on large real-world graphs using shared-memory parallelism (OpenMP) and direction-optimizing traversal. The central research question is:

> How do hybrid switching parameters (`alpha`, `beta`) affect runtime, scalability, and work-efficiency across different graph structures and directed/undirected semantics?

To answer this, we implemented and evaluated three BFS variants:

- `seq` (sequential top-down baseline),
- `topdown_omp` (parallel top-down),
- `hybrid_omp` (direction-optimizing, switching between top-down and bottom-up).

The final experimental pipeline completed a full run schedule (progress logs reached `run 984/984`) and produced:

- `results/ours_all_graphs.csv` (large per-run dataset),
- `results/comparison/combined_runtime_summary.csv` (merged with reference repo timings),
- `results/figures/` (runtime/speedup/heatmaps/switch/edge-work plots).


## 2) Problem We Were Tackling

### 2.1 Practical problem

BFS is fundamental for graph analytics, but performance is highly sensitive to:

- graph topology (diameter, degree distribution, frontier growth),
- traversal direction strategy,
- parallel overhead and load balance,
- and when/if to switch between top-down and bottom-up modes.

Static one-mode BFS (`top-down only`) can perform unnecessary edge checks on power-law or dense-frontier phases. Hybrid direction-optimizing BFS can reduce work, but only if switching thresholds are tuned well.

### 2.2 Technical unknowns we wanted to resolve

1. Does `hybrid_omp` consistently outperform plain `topdown_omp` on these graphs?
2. Which `alpha`/`beta` combinations are best per graph?
3. How much work reduction (edge examinations) is achieved by hybrid mode?
4. How does scaling with threads behave across graph families?
5. How does our implementation compare against the reference CPU implementations (`wbfs`, `qbfs`, `hybrid`)?


## 3) Approach and Hypotheses

### 3.1 High-level approach

We built a controlled experimental framework that:

- runs all BFS modes on multiple large benchmark graphs,
- sweeps a grid of hybrid parameters (`alpha` and `beta`),
- captures runtime + CPU time + switching behavior + examined-edge counts,
- and generates both machine-readable CSV summaries and visualization plots.

### 3.2 Working hypotheses

- H1: `hybrid_omp` should reduce edge examinations significantly vs top-down.
- H2: best `alpha`/`beta` values are graph-dependent, not universal.
- H3: speedup vs sequential should improve up to a moderate thread count, then may taper due to overhead/synchronization and memory pressure.


## 4) Implementation Details

## 4.1 BFS modes

### Sequential (`seq`)

- Standard queue-based BFS used as the correctness and speedup baseline.

### Parallel Top-Down (`topdown_omp`)

- OpenMP-based frontier expansion.
- Per-thread local frontier handling and merge.
- No bottom-up phase.

### Hybrid Direction-Optimizing (`hybrid_omp`)

- Dynamically switches between:
  - top-down expansion,
  - bottom-up scanning,
  - then possibly switches back.
- Controlled by:
  - `alpha`: trigger threshold for top-down -> bottom-up,
  - `beta`: threshold for bottom-up -> top-down.

The implementation records switch levels (`switch_to_bottom_up_level`, `switch_back_to_top_down_level`) and work statistics (`total_edge_examinations`) for analysis.

## 4.2 Graph semantics support

- `--directed=0`: undirected interpretation (symmetric adjacency),
- `--directed=1`: directed interpretation with reverse CSR constructed for bottom-up checks.

## 4.3 Experiment automation

Primary automation scripts:

- `scripts/run_all_graphs_ours.sh`
- `scripts/compare_with_reference.sh`
- `scripts/summarize_comparison.py`
- `scripts/plot_results.py`

Make targets used:

- `make run-graphs` / `make run-graphs-fast`
- `make compare-all` / `make compare-fast`
- `make sync-wsl` (Windows -> WSL Linux copy + rebuild)
- `make sync-results` (WSL results -> Windows workspace)

## 4.4 Performance engineering workflow

To avoid WSL `/mnt/c` filesystem I/O overhead, runs were moved to Linux-native storage (`~/PDC`). This materially reduced per-run wall time and made long sweeps feasible.


## 5) Experimental Setup

## 5.1 Graph set

Reference graph family used in this study:

- `coPapersDBLP`
- `wiki-topcats`
- `com-dblp.ungraph`
- `soc-Epinions1`

## 5.2 Parameter sweep

Default hybrid sweep:

- `alpha in {4, 6, 8, 10, 12, 14, 16, 20}`
- `beta in {8, 16, 24, 32, 48, 64}`

Thread counts: `1, 2, 4, 8, 16`.

## 5.3 Produced artifacts

- `results/ours_all_graphs.csv`  
  Contains mode, threads, alpha/beta, runtime, CPU time, edge examinations, levels, switch points, speedup, efficiency.

- `results/comparison/reference_logs/*.log`  
  Raw timing outputs from reference binaries.

- `results/comparison/combined_runtime_summary.csv`  
  Merged view of reference + our implementation.

- `results/figures/*.png`  
  Plots for runtime scaling, speedup, edge work, alpha-beta heatmaps, switch behavior.


## 6) Results

### 6.1 Dataset scale and completeness

- `compare-all` completed full progress to `run 984/984` in logs.
- `ours_all_graphs.csv` contains 1946 measured rows in the completed output.
- `combined_runtime_summary.csv` contains:
  - 1971 total rows,
  - 1946 rows from our runs,
  - 25 rows from reference logs (for graphs where reference completed in that run).

### 6.2 Best observed runtimes (our implementation)

From `results/ours_all_graphs.csv`, best observed per graph:

- `coPapersDBLP`: `hybrid_omp`, `threads=8`, `alpha=6`, `beta=64`, runtime `0.011683 s`
- `com-dblp.ungraph`: `hybrid_omp`, `threads=8`, `alpha=10`, `beta=32`, runtime `0.005985 s`
- `soc-Epinions1`: `hybrid_omp`, `threads=4`, `alpha=14`, `beta=8`, runtime `0.001147 s`
- `wiki-topcats`: `hybrid_omp`, `threads=8`, `alpha=8`, `beta=24`, runtime `0.038169 s`

Observation: all per-graph best runs came from `hybrid_omp`, supporting the direction-optimizing strategy for this dataset.

### 6.3 Work reduction vs top-down

Comparing `topdown_omp` vs best `hybrid_omp` edge-examination behavior (thread-1 work view):

- `coPapersDBLP`: ~`74.60%` fewer edge examinations
- `com-dblp.ungraph`: ~`51.95%` fewer
- `soc-Epinions1`: ~`67.66%` fewer
- `wiki-topcats`: ~`75.02%` fewer

This directly validates the expected work-efficiency benefit of bottom-up phases on suitable frontier structures.

### 6.4 Speedup highlights

- Maximum observed speedup in our dataset:  
  `9.0849x` on `wiki-topcats` using `hybrid_omp`, `threads=8`, `alpha=8`, `beta=24`.

Thread scaling is generally positive up to mid/high thread counts, but not always monotonic at the highest threads due to memory and synchronization effects (also visible in speedup plots).

### 6.5 Comparison against reference runs

In the completed comparison artifact, reference logs were available for `coPapersDBLP` and `wiki-topcats` in that run. Best observed comparisons:

- `coPapersDBLP`:
  - best ours: `0.011683 s`
  - best reference: `0.022873 s` (`wbfs`, 16 threads)
  - reference/ours ratio: `1.958`

- `wiki-topcats`:
  - best ours: `0.038169 s`
  - best reference: `0.097524 s` (`qbfs`, 4 threads)
  - reference/ours ratio: `2.555`



## 7) Figure Guide (What each plot proves)

Generated in `results/figures/`:

- `runtime_threads_<graph>.png`  
  Shows runtime scaling by mode and thread count.

- `speedup_threads_<graph>.png`  
  Shows actual speedup vs ideal line.

- `edge_exams_<graph>.png`  
  Shows how hybrid reduces total edge examinations relative to top-down.

- `heatmap_alpha_beta_<graph>.png`  
  Shows runtime sensitivity across parameter grid; identifies good `alpha`/`beta` regions.

- `switch_level_alpha_<graph>.png`  
  Shows how switch timing shifts with `alpha`.


## 8) Interpretation and Key Findings

1. **Hybrid wins strongly on this workload**  
   Across best-case runs, `hybrid_omp` is consistently the fastest mode.

2. **Parameter choice matters materially**  
   Heatmaps show that poor `alpha`/`beta` choices can degrade runtime even with the same thread count and graph.

3. **Edge-work reduction is the core mechanism**  
   Large reductions in examined edges (often >50%, up to ~75%) align with runtime improvements.

4. **Graph-specific tuning is important**  
   Best parameters differ by graph; there is no single universally optimal pair.

5. **Throughput gains are real but bounded by hardware effects**  
   Speedup improves significantly, but high-thread behavior can show diminishing returns or regressions.


## 9) Limitations and Validity Notes

- Reference comparison rows were partial in this specific completed summary (25 rows), so baseline conclusions should be interpreted with that scope.
- Absolute runtime values can vary with system load, CPU frequency scaling, and OS scheduling.
- WSL storage path choice has large wall-time impact (`/mnt/c` vs Linux-native filesystem), which was mitigated by running from `~/PDC`.
- Single-source BFS per graph is useful for controlled comparison but can be extended with multi-source robustness sweeps.


## 10) Reproducibility Checklist

From WSL:

```bash
cd /mnt/c/Users/rayaan/Desktop/PDC/direction-optimizing-bfs-heuristic-study
make sync-wsl
cd ~/PDC/direction-optimizing-bfs-heuristic-study
make compare-all
python scripts/plot_results.py results/comparison/ours.csv results/figures
make sync-results
```

Expected key outputs:

- `results/ours_all_graphs.csv`
- `results/comparison/ours.csv`
- `results/comparison/reference_logs/*.log`
- `results/comparison/combined_runtime_summary.csv`
- `results/figures/*.png`


## 11) Future Improvements

- Add automatic parameter-selection heuristics learned from graph statistics.
- Expand reference comparison coverage to all graphs and all variants consistently.
- Add statistical repeat runs (e.g., 5x per config) and confidence intervals.
- Add NUMA-aware placement and thread pinning experiments.
- Add additional real-world graph families for stronger generalization claims.


## 12) Conclusion

The study successfully demonstrates that direction-optimizing hybrid BFS, when combined with parameter sweeps over `alpha`/`beta`, can produce substantial runtime and work-efficiency gains on large graphs. The implementation, automation, and generated artifacts provide a complete technical basis for reporting algorithmic behavior, reproducibility, and comparative performance.

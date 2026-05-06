#!/usr/bin/env python3
"""
Plotting script for direction-optimizing BFS heuristic study.

Usage:
    python scripts/plot_results.py results/sweep_results.csv [output_dir]

Generates:
    1. Runtime vs threads (speedup curve)
    2. Speedup vs threads
    3. Edge examinations: top-down vs hybrid
    4. Runtime heatmap over alpha-beta grid
    5. Switch level vs alpha
    6. Frontier size by BFS level (from per-level CSV if available)
    7. Directed vs undirected comparison
"""

import sys
import os
import pandas as pd
import numpy as np
import matplotlib.pyplot as plt
import matplotlib
matplotlib.use("Agg")

try:
    import seaborn as sns
    HAS_SEABORN = True
except ImportError:
    HAS_SEABORN = False


def load_data(csv_path):
    df = pd.read_csv(csv_path)
    return df


def plot_runtime_vs_threads(df, out_dir):
    """Runtime vs thread count, one line per mode, grouped by graph."""
    graphs = df["graph_name"].unique()
    for gname in graphs:
        gdf = df[df["graph_name"] == gname]
        fig, ax = plt.subplots(figsize=(8, 5))
        for mode in gdf["mode"].unique():
            mdf = gdf[gdf["mode"] == mode]
            means = mdf.groupby("threads")["runtime_seconds"].mean()
            ax.plot(means.index, means.values, "o-", label=mode)
        ax.set_xlabel("Threads")
        ax.set_ylabel("Runtime (s)")
        ax.set_title(f"Runtime vs Threads -- {gname}")
        ax.legend()
        ax.grid(True, alpha=0.3)
        fig.tight_layout()
        fig.savefig(os.path.join(out_dir, f"runtime_threads_{gname}.png"), dpi=150)
        plt.close(fig)


def plot_speedup_vs_threads(df, out_dir):
    """Speedup vs thread count."""
    graphs = df["graph_name"].unique()
    for gname in graphs:
        gdf = df[df["graph_name"] == gname]
        fig, ax = plt.subplots(figsize=(8, 5))
        for mode in gdf["mode"].unique():
            mdf = gdf[gdf["mode"] == mode]
            means = mdf.groupby("threads")["speedup"].mean()
            ax.plot(means.index, means.values, "o-", label=mode)
        # ideal line
        threads = sorted(gdf["threads"].unique())
        ax.plot(threads, threads, "k--", alpha=0.4, label="ideal")
        ax.set_xlabel("Threads")
        ax.set_ylabel("Speedup")
        ax.set_title(f"Speedup vs Threads -- {gname}")
        ax.legend()
        ax.grid(True, alpha=0.3)
        fig.tight_layout()
        fig.savefig(os.path.join(out_dir, f"speedup_threads_{gname}.png"), dpi=150)
        plt.close(fig)


def plot_edge_examinations(df, out_dir):
    """Compare edge examinations between top-down and hybrid."""
    graphs = df["graph_name"].unique()
    for gname in graphs:
        gdf = df[df["graph_name"] == gname]
        modes = [m for m in ["topdown_omp", "hybrid_omp"] if m in gdf["mode"].values]
        if len(modes) < 2:
            continue
        fig, ax = plt.subplots(figsize=(8, 5))
        thread_vals = sorted(gdf["threads"].unique())
        width = 0.35
        x = np.arange(len(thread_vals))
        for i, mode in enumerate(modes):
            mdf = gdf[gdf["mode"] == mode]
            means = mdf.groupby("threads")["total_edge_examinations"].mean()
            vals = [means.get(t, 0) for t in thread_vals]
            ax.bar(x + i * width, vals, width, label=mode)
        ax.set_xlabel("Threads")
        ax.set_ylabel("Edge Examinations")
        ax.set_title(f"Edge Examinations -- {gname}")
        ax.set_xticks(x + width / 2)
        ax.set_xticklabels(thread_vals)
        ax.legend()
        ax.grid(True, alpha=0.3, axis="y")
        fig.tight_layout()
        fig.savefig(os.path.join(out_dir, f"edge_exams_{gname}.png"), dpi=150)
        plt.close(fig)


def plot_alpha_beta_heatmap(df, out_dir):
    """Runtime heatmap over alpha x beta for hybrid mode, per graph."""
    hybrid = df[df["mode"] == "hybrid_omp"]
    if hybrid.empty:
        return
    graphs = hybrid["graph_name"].unique()
    for gname in graphs:
        gdf = hybrid[hybrid["graph_name"] == gname]
        pivot = gdf.pivot_table(
            values="runtime_seconds",
            index="beta",
            columns="alpha",
            aggfunc="mean"
        )
        if pivot.empty:
            continue
        fig, ax = plt.subplots(figsize=(10, 6))
        if HAS_SEABORN:
            sns.heatmap(pivot, annot=True, fmt=".4f", cmap="YlOrRd",
                        ax=ax, cbar_kws={"label": "Runtime (s)"})
        else:
            im = ax.imshow(pivot.values, cmap="YlOrRd", aspect="auto")
            ax.set_xticks(range(len(pivot.columns)))
            ax.set_xticklabels(pivot.columns)
            ax.set_yticks(range(len(pivot.index)))
            ax.set_yticklabels(pivot.index)
            plt.colorbar(im, ax=ax, label="Runtime (s)")
        ax.set_xlabel("Alpha")
        ax.set_ylabel("Beta")
        ax.set_title(f"Runtime Heatmap (alpha x beta) -- {gname}")
        fig.tight_layout()
        fig.savefig(os.path.join(out_dir, f"heatmap_alpha_beta_{gname}.png"), dpi=150)
        plt.close(fig)


def plot_switch_level_vs_alpha(df, out_dir):
    """Switch-to-bottom-up level vs alpha."""
    hybrid = df[(df["mode"] == "hybrid_omp") & (df["switch_to_bottom_up_level"] >= 0)]
    if hybrid.empty:
        return
    graphs = hybrid["graph_name"].unique()
    for gname in graphs:
        gdf = hybrid[hybrid["graph_name"] == gname]
        fig, ax = plt.subplots(figsize=(8, 5))
        means = gdf.groupby("alpha")["switch_to_bottom_up_level"].mean()
        ax.plot(means.index, means.values, "o-", color="tab:blue")
        ax.set_xlabel("Alpha")
        ax.set_ylabel("Switch-to-BU Level")
        ax.set_title(f"Switch Level vs Alpha -- {gname}")
        ax.grid(True, alpha=0.3)
        fig.tight_layout()
        fig.savefig(os.path.join(out_dir, f"switch_level_alpha_{gname}.png"), dpi=150)
        plt.close(fig)


def plot_directed_vs_undirected(df, out_dir):
    """Compare runtime for directed vs undirected on same base graph name."""
    # group by base graph name (strip _undir suffix)
    df = df.copy()
    df["base_name"] = df["graph_name"].str.replace("_undir", "", regex=False)
    bases = df["base_name"].unique()
    for base in bases:
        bdf = df[df["base_name"] == base]
        dir_vals = bdf["directed"].unique()
        if len(dir_vals) < 2:
            continue
        fig, ax = plt.subplots(figsize=(8, 5))
        for d in sorted(dir_vals):
            ddf = bdf[bdf["directed"] == d]
            label = "directed" if d else "undirected"
            for mode in ddf["mode"].unique():
                mdf = ddf[ddf["mode"] == mode]
                means = mdf.groupby("threads")["runtime_seconds"].mean()
                ax.plot(means.index, means.values, "o-",
                        label=f"{label}/{mode}")
        ax.set_xlabel("Threads")
        ax.set_ylabel("Runtime (s)")
        ax.set_title(f"Directed vs Undirected -- {base}")
        ax.legend(fontsize=8)
        ax.grid(True, alpha=0.3)
        fig.tight_layout()
        fig.savefig(os.path.join(out_dir, f"dir_vs_undir_{base}.png"), dpi=150)
        plt.close(fig)


def main():
    if len(sys.argv) < 2:
        print("Usage: python scripts/plot_results.py <csv_path> [output_dir]")
        sys.exit(1)

    csv_path = sys.argv[1]
    out_dir = sys.argv[2] if len(sys.argv) > 2 else "results/figures"
    os.makedirs(out_dir, exist_ok=True)

    df = load_data(csv_path)
    print(f"Loaded {len(df)} rows from {csv_path}")

    plot_runtime_vs_threads(df, out_dir)
    print("  [1/6] Runtime vs threads")

    plot_speedup_vs_threads(df, out_dir)
    print("  [2/6] Speedup vs threads")

    plot_edge_examinations(df, out_dir)
    print("  [3/6] Edge examinations comparison")

    plot_alpha_beta_heatmap(df, out_dir)
    print("  [4/6] Alpha-beta heatmap")

    plot_switch_level_vs_alpha(df, out_dir)
    print("  [5/6] Switch level vs alpha")

    plot_directed_vs_undirected(df, out_dir)
    print("  [6/6] Directed vs undirected comparison")

    print(f"\nAll figures saved to {out_dir}/")


if __name__ == "__main__":
    main()

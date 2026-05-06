#!/bin/bash
# Parameter sweep for direction-optimizing BFS heuristic study.
# Usage: bash scripts/run_sweep.sh [csv_output_path]
#
# Requires ./bfs to be built already (make).

set -e

BFS=./bfs
CSV="${1:-results/sweep_results.csv}"

# Write CSV header (overwrites existing file)
mkdir -p "$(dirname "$CSV")"

HEADER="graph_name,graph_family,directed,source,mode,threads,alpha,beta,runtime_seconds,cpu_time_seconds,total_visited,total_levels,total_edge_examinations,switch_to_bottom_up_level,switch_back_to_top_down_level,top_down_levels,bottom_up_levels,frontier_at_switch,visited_fraction_at_switch,speedup,efficiency"
echo "$HEADER" > "$CSV"

# --------------- Configuration ---------------

# Graphs: path|name|family|directed|sources (comma-separated)
GRAPHS=(
    "data/chain.txt|chain|synthetic|0|0,1,2"
    "data/branching.txt|branching|synthetic|0|0"
    "data/directed_small.txt|directed_small|synthetic|1|0"
    "data/directed_small.txt|directed_small_undir|synthetic|0|0"
    # Add real graphs here, for example:
    # "data/soc-Epinions1.txt|soc-Epinions1|directed_social|1|0,100,500,1000,5000"
    # "data/coPapersDBLP.txt|coPapersDBLP|undirected_collab|0|0,100,500,1000,5000"
    # "data/wiki-topcats.txt|wiki-topcats|directed_web|1|0,100,500,1000,5000"
)

THREAD_COUNTS=(1 2 4 8)
ALPHAS=(4 6 8 10 12 14 16 20)
BETAS=(8 16 24 32 48 64)

# --------------- Sweep ---------------

for entry in "${GRAPHS[@]}"; do
    IFS='|' read -r gpath gname gfamily gdirected gsources <<< "$entry"
    IFS=',' read -ra SOURCES <<< "$gsources"

    echo "========================================"
    echo "Graph: $gname ($gpath) directed=$gdirected"
    echo "========================================"

    for src in "${SOURCES[@]}"; do

        # --- Sequential baseline ---
        echo "  [seq] source=$src"
        $BFS "$gpath" --source="$src" --directed="$gdirected" \
            --mode=seq --threads=1 --csv="$CSV" \
            --name="$gname" --family="$gfamily" \
            > /dev/null 2>&1

        for threads in "${THREAD_COUNTS[@]}"; do

            # --- Parallel top-down ---
            echo "  [topdown_omp] source=$src threads=$threads"
            $BFS "$gpath" --source="$src" --directed="$gdirected" \
                --mode=topdown_omp --threads="$threads" --csv="$CSV" \
                --name="$gname" --family="$gfamily" \
                > /dev/null 2>&1

            # --- Hybrid sweep over alpha x beta ---
            for alpha in "${ALPHAS[@]}"; do
                for beta in "${BETAS[@]}"; do
                    echo "  [hybrid_omp] source=$src threads=$threads alpha=$alpha beta=$beta"
                    $BFS "$gpath" --source="$src" --directed="$gdirected" \
                        --mode=hybrid_omp --threads="$threads" \
                        --alpha="$alpha" --beta="$beta" --csv="$CSV" \
                        --name="$gname" --family="$gfamily" \
                        > /dev/null 2>&1
                done
            done
        done
    done
done

echo ""
echo "Sweep complete. Results in: $CSV"
echo "Total rows: $(( $(wc -l < "$CSV") - 1 ))"

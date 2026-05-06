#!/bin/bash
# Run this implementation on all graph files in the reference Graphs folder.
# Output CSV: results/ours_all_graphs.csv
#
# Fast mode:
#   FAST=1 bash scripts/run_all_graphs_ours.sh
#   - drops sequential runs on very large graphs (wiki-topcats, coPapersDBLP)
#   - uses fewer thread counts: 4,8,16

set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT_DIR"

BFS="./bfs"
REF_GRAPHS_DIR="../Parallel-Breadth-First-Search-OpenMP-and-CUDA/Graphs"
OUT_CSV="results/ours_all_graphs.csv"
LOG_DIR="results/logs/ours_all_graphs"

mkdir -p results
mkdir -p "$LOG_DIR"
rm -f "$OUT_CSV"

if [[ ! -x "$BFS" ]]; then
  echo "Missing $BFS. Run: make"
  exit 1
fi

# Graph path list from the reference Graphs folder
GRAPHS=(
  "$REF_GRAPHS_DIR/coPapersDBLP/coPapersDBLP.mtx"
  "$REF_GRAPHS_DIR/wiki-topcats/wiki-topcats.mtx"
  "$REF_GRAPHS_DIR/com-dblp.ungraph.txt/com-dblp.ungraph.txt"
  "$REF_GRAPHS_DIR/soc-Epinions1.txt/soc-Epinions1.txt"
)

THREADS=(1 2 4 8 16)
MODES=(seq topdown_omp hybrid_omp)
FAST="${FAST:-0}"
VERBOSE="${VERBOSE:-1}"
SWEEP_ALPHA_BETA="${SWEEP_ALPHA_BETA:-1}"
ALPHAS_CSV="${ALPHAS_CSV:-4,6,8,10,12,14,16,20}"
BETAS_CSV="${BETAS_CSV:-8,16,24,32,48,64}"

timestamp() {
  date +"%Y-%m-%d %H:%M:%S"
}

log() {
  echo "[$(timestamp)] $*"
}

split_csv_to_array() {
  local csv="$1"
  local -n arr_ref=$2
  arr_ref=()
  IFS=',' read -r -a arr_ref <<< "$csv"
}

if [[ "$FAST" == "1" ]]; then
  THREADS=(4 8 16)
fi

if [[ "$SWEEP_ALPHA_BETA" == "1" ]]; then
  split_csv_to_array "$ALPHAS_CSV" ALPHAS
  split_csv_to_array "$BETAS_CSV" BETAS
else
  ALPHAS=(14)
  BETAS=(24)
fi

TOTAL_RUNS=0
for g in "${GRAPHS[@]}"; do
  base="$(basename "$g")"
  is_large=0
  if [[ "$base" == "wiki-topcats.mtx" || "$base" == "coPapersDBLP.mtx" ]]; then
    is_large=1
  fi
  for mode in "${MODES[@]}"; do
    for t in "${THREADS[@]}"; do
      if [[ "$mode" == "seq" && "$t" != "1" ]]; then
        continue
      fi
      if [[ "$FAST" == "1" && "$is_large" == "1" && "$mode" == "seq" ]]; then
        continue
      fi
      if [[ "$mode" == "hybrid_omp" ]]; then
        TOTAL_RUNS=$((TOTAL_RUNS + ${#ALPHAS[@]} * ${#BETAS[@]}))
      else
        TOTAL_RUNS=$((TOTAL_RUNS + 1))
      fi
    done
  done
done

log "Starting run_all_graphs_ours.sh"
log "FAST=$FAST VERBOSE=$VERBOSE"
log "SWEEP_ALPHA_BETA=$SWEEP_ALPHA_BETA"
if [[ "$SWEEP_ALPHA_BETA" == "1" ]]; then
  log "Hybrid alpha list: ${ALPHAS[*]}"
  log "Hybrid beta list:  ${BETAS[*]}"
fi
log "Output CSV: $OUT_CSV"
log "Log directory: $LOG_DIR"
log "Planned runs: $TOTAL_RUNS"

RUN_ID=0

for g in "${GRAPHS[@]}"; do
  if [[ ! -f "$g" ]]; then
    echo "Skipping missing graph: $g"
    continue
  fi

  base="$(basename "$g")"
  name="${base%.*}"
  family="reference_graphs"
  directed=1

  # Known undirected datasets
  if [[ "$base" == "coPapersDBLP.mtx" || "$base" == "com-dblp.ungraph.txt" ]]; then
    directed=0
  fi

  log "============================================================"
  log "Graph: $g"
  log "Name: $name directed=$directed"
  log "============================================================"

  is_large=0
  if [[ "$base" == "wiki-topcats.mtx" || "$base" == "coPapersDBLP.mtx" ]]; then
    is_large=1
  fi

  for mode in "${MODES[@]}"; do
    for t in "${THREADS[@]}"; do
      # sequential should run once with threads=1
      if [[ "$mode" == "seq" && "$t" != "1" ]]; then
        continue
      fi
      # fast mode: skip sequential on very large graphs
      if [[ "$FAST" == "1" && "$is_large" == "1" && "$mode" == "seq" ]]; then
        continue
      fi

      if [[ "$mode" == "hybrid_omp" ]]; then
        for alpha in "${ALPHAS[@]}"; do
          for beta in "${BETAS[@]}"; do
            RUN_ID=$((RUN_ID + 1))
            safe_name="${name//[^a-zA-Z0-9_.-]/_}"
            run_log="$LOG_DIR/${RUN_ID}_${safe_name}_${mode}_t${t}_a${alpha}_b${beta}.log"
            log "[run $RUN_ID/$TOTAL_RUNS] graph=$name mode=$mode threads=$t alpha=$alpha beta=$beta log=$run_log"

            cmd=( "$BFS" "$g"
              --source=0 --directed="$directed"
              --mode="$mode" --threads="$t"
              --alpha="$alpha" --beta="$beta"
              --csv="$OUT_CSV" --name="$name" --family="$family"
            )

            start_ts=$(date +%s)
            if [[ "$VERBOSE" == "2" ]]; then
              "${cmd[@]}" 2>&1 | tee "$run_log"
            elif [[ "$VERBOSE" == "1" ]]; then
              "${cmd[@]}" > "$run_log" 2>&1
            else
              "${cmd[@]}" > /dev/null 2>&1
            fi
            end_ts=$(date +%s)
            elapsed=$((end_ts - start_ts))
            log "[run $RUN_ID/$TOTAL_RUNS] completed in ${elapsed}s"
          done
        done
      else
        RUN_ID=$((RUN_ID + 1))
        safe_name="${name//[^a-zA-Z0-9_.-]/_}"
        run_log="$LOG_DIR/${RUN_ID}_${safe_name}_${mode}_t${t}.log"
        log "[run $RUN_ID/$TOTAL_RUNS] graph=$name mode=$mode threads=$t log=$run_log"

        cmd=( "$BFS" "$g"
          --source=0 --directed="$directed"
          --mode="$mode" --threads="$t"
          --csv="$OUT_CSV" --name="$name" --family="$family"
        )

        start_ts=$(date +%s)
        if [[ "$VERBOSE" == "2" ]]; then
          "${cmd[@]}" 2>&1 | tee "$run_log"
        elif [[ "$VERBOSE" == "1" ]]; then
          "${cmd[@]}" > "$run_log" 2>&1
        else
          "${cmd[@]}" > /dev/null 2>&1
        fi
        end_ts=$(date +%s)
        elapsed=$((end_ts - start_ts))
        log "[run $RUN_ID/$TOTAL_RUNS] completed in ${elapsed}s"
      fi
    done
  done
done

log "Done. Wrote: $OUT_CSV"
log "Rows: $(( $(wc -l < "$OUT_CSV") - 1 ))"

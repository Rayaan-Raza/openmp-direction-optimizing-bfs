#!/bin/bash
# Build and run both:
#   1) Reference CPU BFS repo
#   2) This implementation
# Then write a combined runtime summary CSV.

set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT_DIR"

REF_ROOT="../Parallel-Breadth-First-Search-OpenMP-and-CUDA"
REF_CPU="$REF_ROOT/CPU"
OUT_DIR="results/comparison"
REF_LOG_DIR="$OUT_DIR/reference_logs"
OURS_CSV="$OUT_DIR/ours.csv"
SUMMARY_CSV="$OUT_DIR/combined_runtime_summary.csv"
REF_LOG_DIR_ABS=""
FAST="${FAST:-0}"
VERBOSE="${VERBOSE:-1}"
SWEEP_ALPHA_BETA="${SWEEP_ALPHA_BETA:-1}"

timestamp() {
  date +"%Y-%m-%d %H:%M:%S"
}

log() {
  echo "[$(timestamp)] $*"
}

mkdir -p "$REF_LOG_DIR"
rm -f "$OURS_CSV" "$SUMMARY_CSV"
rm -f "$REF_LOG_DIR"/*.log
REF_LOG_DIR_ABS="$(cd "$REF_LOG_DIR" && pwd)"

if [[ ! -x ./bfs ]]; then
  log "Missing ./bfs. Building..."
  make
fi

log "Starting compare_with_reference.sh"
log "FAST=$FAST VERBOSE=$VERBOSE"
log "SWEEP_ALPHA_BETA=$SWEEP_ALPHA_BETA"
log "Building reference CPU binaries..."
make -C "$REF_CPU" wbfs qbfs hybrid

run_reference() {
  local variant="$1"
  local graph="$2"
  local log_file="$3"
  local start_ts
  local end_ts
  local elapsed
  local rc
  log "[reference] variant=$variant graph=$graph log=$log_file"
  start_ts=$(date +%s)
  if [[ "$VERBOSE" == "2" ]]; then
    if (
      cd "$REF_CPU"
      "./$variant" "$graph" 0
    ) 2>&1 | tee "$log_file"; then
      rc=0
    else
      rc=$?
    fi
  else
    if (
      cd "$REF_CPU"
      "./$variant" "$graph" 0 > "$log_file" 2>&1
    ); then
      rc=0
    else
      rc=$?
    fi
  fi
  end_ts=$(date +%s)
  elapsed=$((end_ts - start_ts))
  if [[ $rc -eq 0 ]]; then
    log "[reference] completed in ${elapsed}s"
  else
    log "[reference] WARNING: failed (exit=$rc) after ${elapsed}s; continuing"
  fi
}

REF_GRAPHS=(
  "../Graphs/coPapersDBLP/coPapersDBLP.mtx"
  "../Graphs/wiki-topcats/wiki-topcats.mtx"
)

# Fast mode: skip the slowest reference graph.
if [[ "$FAST" == "1" ]]; then
  REF_GRAPHS=("../Graphs/coPapersDBLP/coPapersDBLP.mtx")
fi

log "Running reference implementations..."
for graph in "${REF_GRAPHS[@]}"
do
  graph_name="$(basename "$graph")"
  run_reference "wbfs" "$graph" "$REF_LOG_DIR_ABS/wbfs__${graph_name}.log"
  run_reference "qbfs" "$graph" "$REF_LOG_DIR_ABS/qbfs__${graph_name}.log"
  run_reference "hybrid" "$graph" "$REF_LOG_DIR_ABS/hybrid__${graph_name}.log"
done

log "Running this implementation across all reference graphs..."
if [[ "$FAST" == "1" && "$SWEEP_ALPHA_BETA" == "1" ]]; then
  # smaller sweep in fast mode
  FAST="$FAST" VERBOSE="$VERBOSE" SWEEP_ALPHA_BETA="$SWEEP_ALPHA_BETA" \
    ALPHAS_CSV="8,14,20" BETAS_CSV="16,24,48" \
    bash scripts/run_all_graphs_ours.sh
else
  FAST="$FAST" VERBOSE="$VERBOSE" SWEEP_ALPHA_BETA="$SWEEP_ALPHA_BETA" \
    bash scripts/run_all_graphs_ours.sh
fi
cp results/ours_all_graphs.csv "$OURS_CSV"

log "Creating combined summary CSV..."
python3 scripts/summarize_comparison.py "$REF_LOG_DIR" "$OURS_CSV" "$SUMMARY_CSV"

log "Done."
log "Ours CSV:       $OURS_CSV"
log "Reference logs: $REF_LOG_DIR/"
log "Summary CSV:    $SUMMARY_CSV"

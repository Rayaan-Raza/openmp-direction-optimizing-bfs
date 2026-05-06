CXX      := g++
CXXFLAGS := -std=c++17 -O3 -fopenmp -Wall -Wextra
INCDIR   := include
SRCDIR   := src
BUILDDIR := build
TARGET   := bfs
REF_ROOT := ../Parallel-Breadth-First-Search-OpenMP-and-CUDA
REF_CPU  := $(REF_ROOT)/CPU
REF_GRAPHS := $(REF_ROOT)/Graphs

SRCS := $(wildcard $(SRCDIR)/*.cpp)
OBJS := $(patsubst $(SRCDIR)/%.cpp,$(BUILDDIR)/%.o,$(SRCS))

.PHONY: all clean test test-omp test-hybrid test-all \
        run-graphs ours-all run-graphs-fast compare-all compare-fast \
        run-graphs-debug compare-debug comapre-all reference-build \
        sync-wsl sync-results

# Sync from Windows drive (WSL /mnt/c/...) to Linux $HOME, then rebuild there.
# Override paths: make sync-wsl WSL_SYNC_SRC=/mnt/c/Users/you/Desktop/PDC
WSL_SYNC_SRC  ?= /mnt/c/Users/rayaan/Desktop/PDC
WSL_SYNC_DEST ?= $(HOME)/PDC
WSL_RESULTS_SRC  ?= $(WSL_SYNC_DEST)/direction-optimizing-bfs-heuristic-study/results
WSL_RESULTS_DEST ?= /mnt/c/Users/rayaan/Desktop/PDC/direction-optimizing-bfs-heuristic-study/results

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

$(BUILDDIR)/%.o: $(SRCDIR)/%.cpp | $(BUILDDIR)
	$(CXX) $(CXXFLAGS) -I$(INCDIR) -c -o $@ $<

$(BUILDDIR):
	mkdir -p $(BUILDDIR)

clean:
	rm -rf $(BUILDDIR) $(TARGET)

# ---- Correctness tests ----

test: $(TARGET)
	@echo "=== Test 1: Chain (undirected, source 0) ==="
	./$(TARGET) data/chain.txt --source=0 --directed=0 --mode=seq
	@echo ""
	@echo "=== Test 2: Branching (undirected, source 0) ==="
	./$(TARGET) data/branching.txt --source=0 --directed=0 --mode=seq
	@echo ""
	@echo "=== Test 3a: Directed small (directed, source 0) ==="
	./$(TARGET) data/directed_small.txt --source=0 --directed=1 --mode=seq
	@echo ""
	@echo "=== Test 3b: Directed small (undirected, source 0) ==="
	./$(TARGET) data/directed_small.txt --source=0 --directed=0 --mode=seq
	@echo ""

test-omp: $(TARGET)
	@echo "=== OMP Top-Down: Chain (verify vs seq) ==="
	./$(TARGET) data/chain.txt --source=0 --directed=0 --mode=topdown_omp --threads=2 --verify
	@echo ""
	@echo "=== OMP Top-Down: Branching (verify vs seq) ==="
	./$(TARGET) data/branching.txt --source=0 --directed=0 --mode=topdown_omp --threads=2 --verify
	@echo ""
	@echo "=== OMP Top-Down: Directed small directed (verify vs seq) ==="
	./$(TARGET) data/directed_small.txt --source=0 --directed=1 --mode=topdown_omp --threads=2 --verify
	@echo ""

test-hybrid: $(TARGET)
	@echo "=== Hybrid: Chain (verify vs seq) ==="
	./$(TARGET) data/chain.txt --source=0 --directed=0 --mode=hybrid_omp --threads=2 --alpha=6 --beta=24 --verify
	@echo ""
	@echo "=== Hybrid: Branching (verify vs seq) ==="
	./$(TARGET) data/branching.txt --source=0 --directed=0 --mode=hybrid_omp --threads=2 --alpha=6 --beta=24 --verify
	@echo ""
	@echo "=== Hybrid: Directed small directed (verify vs seq) ==="
	./$(TARGET) data/directed_small.txt --source=0 --directed=1 --mode=hybrid_omp --threads=2 --alpha=6 --beta=24 --verify
	@echo ""

test-all: test test-omp test-hybrid

# ---- Full graph runs (this repo) ----
# Runs every graph file under ../Parallel-Breadth-First-Search-OpenMP-and-CUDA/Graphs
# and writes CSV to results/ours_all_graphs.csv.
run-graphs: $(TARGET)
	@bash scripts/run_all_graphs_ours.sh

ours-all: run-graphs

run-graphs-fast: $(TARGET)
	@FAST=1 SWEEP_ALPHA_BETA=1 bash scripts/run_all_graphs_ours.sh

run-graphs-debug: $(TARGET)
	@VERBOSE=2 bash scripts/run_all_graphs_ours.sh

# ---- Reference + ours comparison ----
# Builds and runs both codebases and writes:
#   results/comparison/ours.csv
#   results/comparison/reference_logs/*.log
#   results/comparison/combined_runtime_summary.csv
compare-all: $(TARGET)
	@SWEEP_ALPHA_BETA=1 bash scripts/compare_with_reference.sh

compare-fast: $(TARGET)
	@FAST=1 SWEEP_ALPHA_BETA=1 bash scripts/compare_with_reference.sh

compare-debug: $(TARGET)
	@VERBOSE=2 SWEEP_ALPHA_BETA=1 bash scripts/compare_with_reference.sh

# Compatibility alias for common typo.
comapre-all: compare-all

reference-build:
	$(MAKE) -C "$(REF_CPU)" wbfs qbfs hybrid

# ---- WSL: copy repo to Linux filesystem (run inside WSL), then make all in ~/PDC copy ----

sync-wsl:
	@WSL_SYNC_SRC='$(WSL_SYNC_SRC)' WSL_SYNC_DEST='$(WSL_SYNC_DEST)' bash scripts/sync_from_windows_to_wsl.sh
	@find '$(WSL_SYNC_DEST)/direction-optimizing-bfs-heuristic-study/scripts' -type f -name '*.sh' -exec sed -i 's/\r$$//' {} \;
	@$(MAKE) -C '$(WSL_SYNC_DEST)/direction-optimizing-bfs-heuristic-study' all

# Copy generated results back from WSL Linux copy to Windows workspace.
sync-results:
	@mkdir -p '$(WSL_RESULTS_DEST)'
	@rsync -a '$(WSL_RESULTS_SRC)/' '$(WSL_RESULTS_DEST)/'
	@echo "Synced results to $(WSL_RESULTS_DEST)"

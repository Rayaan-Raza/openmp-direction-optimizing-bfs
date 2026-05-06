#!/usr/bin/env python3
"""
Build a combined runtime summary from:
  - reference logs (wbfs/qbfs/hybrid output)
  - this implementation CSV
"""

import csv
import os
import re
import sys
from pathlib import Path


def parse_reference_logs(log_dir: Path):
    rows = []
    # Example line:
    # Threads: 8    Level: 7    Delta: 123   Time: 0.4567
    line_re = re.compile(
        r"Threads:\s*(\d+)\s+Level:\s*([-\d]+)\s+Delta:\s*([-\d]+)\s+Time:\s*([0-9.eE+-]+)"
    )

    for log_file in sorted(log_dir.glob("*.log")):
        name = log_file.stem  # variant__graph
        if "__" in name:
            variant, graph_name = name.split("__", 1)
        else:
            variant, graph_name = "unknown", name

        with log_file.open("r", encoding="utf-8", errors="ignore") as f:
            for line in f:
                m = line_re.search(line)
                if not m:
                    continue
                threads = int(m.group(1))
                level = int(m.group(2))
                delta = int(m.group(3))
                runtime = float(m.group(4))
                rows.append(
                    {
                        "impl": "reference",
                        "graph_name": graph_name,
                        "variant_or_mode": variant,
                        "threads": threads,
                        "runtime_seconds": runtime,
                        "total_levels": level,
                        "delta_unreached": delta,
                        "source": "",
                        "directed": "",
                        "alpha": "",
                        "beta": "",
                    }
                )
    return rows


def parse_ours_csv(ours_csv: Path):
    rows = []
    with ours_csv.open("r", encoding="utf-8") as f:
        reader = csv.DictReader(f)
        for r in reader:
            rows.append(
                {
                    "impl": "ours",
                    "graph_name": r.get("graph_name", ""),
                    "variant_or_mode": r.get("mode", ""),
                    "threads": r.get("threads", ""),
                    "runtime_seconds": r.get("runtime_seconds", ""),
                    "total_levels": r.get("total_levels", ""),
                    "delta_unreached": "",
                    "source": r.get("source", ""),
                    "directed": r.get("directed", ""),
                    "alpha": r.get("alpha", ""),
                    "beta": r.get("beta", ""),
                }
            )
    return rows


def main():
    if len(sys.argv) != 4:
        print(
            "Usage: python3 scripts/summarize_comparison.py "
            "<reference_log_dir> <ours_csv> <summary_csv>"
        )
        sys.exit(1)

    ref_dir = Path(sys.argv[1])
    ours_csv = Path(sys.argv[2])
    out_csv = Path(sys.argv[3])
    out_csv.parent.mkdir(parents=True, exist_ok=True)

    rows = []
    if ref_dir.exists():
        rows.extend(parse_reference_logs(ref_dir))
    if ours_csv.exists():
        rows.extend(parse_ours_csv(ours_csv))

    fieldnames = [
        "impl",
        "graph_name",
        "variant_or_mode",
        "threads",
        "runtime_seconds",
        "total_levels",
        "delta_unreached",
        "source",
        "directed",
        "alpha",
        "beta",
    ]

    with out_csv.open("w", newline="", encoding="utf-8") as f:
        writer = csv.DictWriter(f, fieldnames=fieldnames)
        writer.writeheader()
        writer.writerows(rows)

    print(f"Wrote {len(rows)} rows to {out_csv}")


if __name__ == "__main__":
    main()

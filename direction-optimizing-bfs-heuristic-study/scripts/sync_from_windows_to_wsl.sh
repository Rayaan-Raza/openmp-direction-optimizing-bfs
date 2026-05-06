#!/usr/bin/env bash
# Mirror the PDC workspace from the Windows drive (/mnt/c/...) into WSL's Linux
# filesystem for faster graph I/O during benchmarks. Intended to run inside WSL.
set -euo pipefail

SRC="${WSL_SYNC_SRC:-/mnt/c/Users/rayaan/Desktop/PDC}"
DEST="${WSL_SYNC_DEST:-$HOME/PDC}"

if [[ ! -d "$SRC" ]]; then
  echo "sync_from_windows_to_wsl.sh: source directory not found: $SRC" >&2
  echo "Set WSL_SYNC_SRC to your Windows-mounted PDC path (e.g. /mnt/c/Users/you/Desktop/PDC)." >&2
  exit 1
fi

mkdir -p "$DEST"

echo "Syncing:"
echo "  from: $SRC"
echo "  to:   $DEST"

rsync -a --delete "$SRC/" "$DEST/" \
  --exclude node_modules \
  --exclude .venv \
  --exclude __pycache__

echo "Done."

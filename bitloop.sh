#!/usr/bin/env bash
# bitloop (make this executable: chmod +x bitloop)
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
python3 "$SCRIPT_DIR/scripts/cli.py" "$@"

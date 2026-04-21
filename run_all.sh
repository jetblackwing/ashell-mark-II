#!/usr/bin/env bash
# Build and run the part5 shell quickly.
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "$0")" && pwd)"
cd "$ROOT_DIR/part5"

echo "Building part5 shell..."
make

echo "Running shell (binary: ./shell)"
./shell

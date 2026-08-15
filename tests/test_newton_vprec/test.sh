#!/bin/bash
set -e

# Make scripts executable
chmod +x clean.sh piecewise_search.py plot_results.py

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"

if [ -z "$PYTHON" ]; then
    if command -v uv >/dev/null 2>&1; then
        PYTHON="uv run --no-sync python"
    elif [ -f "$REPO_ROOT/.venv/bin/python" ]; then
        PYTHON="$REPO_ROOT/.venv/bin/python"
    else
        PYTHON="python3"
    fi
fi

./clean.sh

echo "=== 0. Running piecewise search regression checks ==="
$PYTHON test_piecewise.py

echo "=== 1. Compiling Newton-Raphson benchmark with Verificarlo ==="
verificarlo-c newton.c -o newton -lm

echo "=== 2. Running IEEE binary64 baseline ==="
VFC_BACKENDS="libinterflop_ieee.so" ./newton 0 15 > ieee.out

# Ensure VPREC backend is loaded during search evaluations
export VFC_BACKENDS="libinterflop_vprec.so --mode=ob"

echo "=== 3. Piecewise Search & Evaluation for tolerance 1e-5 ==="
$PYTHON -m verificarlo.optimize.piecewise -n 10 -o schedule_1e-5.txt -r "./newton 1 5"
VFC_SCHEDULE_FILE=schedule_1e-5.txt ./newton 1 5 > vprec_1e-5.out

echo "=== 4. Piecewise Search & Evaluation for tolerance 1e-10 ==="
$PYTHON -m verificarlo.optimize.piecewise -n 10 -o schedule_1e-10.txt -r "./newton 1 10"
VFC_SCHEDULE_FILE=schedule_1e-10.txt ./newton 1 10 > vprec_1e-10.out

echo "=== 5. Piecewise Search (with Animation 1: Search Process) & Evaluation for tolerance 1e-15 ==="
$PYTHON -m verificarlo.optimize.piecewise -n 10 --animate --animation-file newton_search_animation.gif -o schedule_1e-15.txt -r "./newton 1 15"
VFC_SCHEDULE_FILE=schedule_1e-15.txt ./newton 1 15 > vprec_1e-15.out

echo "=== 6. Generating Unified Dynamic Plot & Animation 2 (Convergence Process) ==="
$PYTHON plot_results.py ieee.out vprec_1e-5.out vprec_1e-10.out vprec_1e-15.out

echo "=== 7. Verification Assertions ==="
grep -q "^9	" ieee.out || (echo "FAILED: IEEE did not complete" && exit 1)
grep -q "^9	" vprec_1e-5.out || (echo "FAILED: VPREC 1e-5 did not complete" && exit 1)
grep -q "^9	" vprec_1e-10.out || (echo "FAILED: VPREC 1e-10 did not complete" && exit 1)
grep -q "^9	" vprec_1e-15.out || (echo "FAILED: VPREC 1e-15 did not complete" && exit 1)

echo "SUCCESS: Newton-Raphson VPREC dynamic experiment & animation pipeline completed successfully!"

#!/usr/bin/env bash
# Runs the five FIR example programs on the single-cycle processor and three pipelines
# with the command-line simulator, and prints instructions and cycles for each run.
# Usage:  scripts/run_fir_benchmarks.sh      (set QT_DIR if Qt 6 is not found automatically)
# (rv5-nh-nf is left out: without hazard detection or forwarding these programs need nops.)
set -euo pipefail
cd "$(dirname "$0")/.."
QT_DIR="${QT_DIR:-}"
cmake -S . -B build/cli -G Ninja -DCMAKE_BUILD_TYPE=Release -DENABLE_TESTS=OFF -DDISABLE_GUI=ON -DLOG_PANEL=OFF ${QT_DIR:+-DCMAKE_PREFIX_PATH="$QT_DIR"}
cmake --build build/cli --target kites_cli --parallel
CLI=$(find build/cli -name kites-cli -type f | head -n 1)
printf "%-20s %-10s %14s %8s\n" program vm instructions cycles
for p in fir_scalar.s fir_vector.s fir_vector_macc.s fir_scalar_fp.s fir_vector_fp.s; do
  for vm in rvss rv5-h-f rv5-h-nf rv5-nh-f; do
    out=$("$CLI" "examples/$p" --vm "$vm" --stats 2>&1)
    instr=$(echo "$out" | sed -n 's/.*Instructions retired: *\([0-9,]*\).*/\1/p' | head -n1)
    cycles=$(echo "$out" | sed -n 's/^ *Cycles: *\([0-9,]*\).*/\1/p' | head -n1)
    printf "%-20s %-10s %14s %8s\n" "$p" "$vm" "$instr" "$cycles"
  done
done

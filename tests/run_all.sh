#!/bin/sh
set -e

ROOT="$(dirname "$0")/.."
cd "$ROOT"

echo "[TEST SUITE] running all smoke tests..."

./tests/run_ui_out_msg.sh
./tests/run_communicator.sh
./tests/run_cryptor.sh

echo "[TEST SUITE] done."

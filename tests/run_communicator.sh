#!/bin/sh
set -e

ROOT="$(dirname "$0")/.."
cd "$ROOT"

echo "[TEST] communicator"

mkdir -p msgs encrypted_msgs keys

# чистим encrypted_msgs
rm -f encrypted_msgs/* || true

# запускаем communicator на 3 секунды
timeout 3 ./bin/communicator || true

echo "[INFO] files created in encrypted_msgs/:"
ls -lt encrypted_msgs || true

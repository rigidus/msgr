#!/bin/sh
set -e

ROOT="$(dirname "$0")/.."
cd "$ROOT"

echo "[TEST] ui_out_msg"

mkdir -p msgs encrypted_msgs keys

# создаём входящее сообщение в msgs/
ts=$(date -u +"%Y-%m-%dT%H-%M-%S.%3NZ")
id="TESTIN01"
name="${ts}__${id}__in"

cat > "msgs/${name}" <<'EOF'
(msg :id "TESTIN01" :time "2025-09-17T00:00:00.000Z" :dir "in" :data "Привет из теста!")
EOF

echo "[INFO] created msgs/$name"

# запускаем ui_out_msg на 2 секунды
timeout 2 ./bin/ui_out_msg || true

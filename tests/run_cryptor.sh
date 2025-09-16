#!/bin/sh
set -e

ROOT="$(dirname "$0")/.."
cd "$ROOT"

echo "[TEST] cryptor (encrypt direction)"

mkdir -p msgs encrypted_msgs keys

# подчистим каталоги
rm -f msgs/* encrypted_msgs/* || true

# создаём открытое исходящее сообщение
ts=$(date -u +"%Y-%m-%dT%H-%M-%S.%3NZ")
id="TESTOUT01"
name="${ts}__${id}__out"

cat > "msgs/${name}" <<'EOF'
(msg :id "TESTOUT01" :time "2025-09-17T00:00:00.000Z" :dir "out" :data "Сообщение для шифрования")
EOF

echo "[INFO] created msgs/$name"

# запускаем cryptor на 3 секунды
timeout 3 ./bin/cryptor || true

echo "[INFO] contents of msgs/:"
ls -l msgs || true
echo "[INFO] contents of encrypted_msgs/:"
ls -l encrypted_msgs || true

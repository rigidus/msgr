#!/bin/sh
set -e

ROOT="$(dirname "$0")/.."
cd "$ROOT"

echo "[TEST] ui_in_msg"

mkdir -p msgs encrypted_msgs keys
rm -f msgs/* || true

# запускаем ui_in_msg в фоне, перенаправив ввод из here-doc
(
  sleep 1
  echo "hello from test 1"
  sleep 1
  echo "привет из теста 2"
  sleep 1
  echo "line with \"quotes\""
) | ./bin/ui_in_msg &
PID=$!

# ждём немного, чтобы процесс успел обработать строки
sleep 5

# отправляем SIGINT (Ctrl+C)
kill -INT $PID
wait $PID || true

echo "[INFO] msgs/ now contains:"
ls -lt msgs || true

echo "[INFO] sample content:"
for f in msgs/*; do
    echo "---- $f"
    cat "$f"
    echo ""
done

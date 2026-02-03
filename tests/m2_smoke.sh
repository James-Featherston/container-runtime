#!/usr/bin/env bash

set -euo pipefail

BIN=${1:-./minijfc}
ROOTFS=${2:-./rootfs}

echo "[m2] exit status passthrough"
set +e
$BIN run --rootfs "$ROOTFS" -- /bin/sh -c 'exit 7'
st=$?
set -e
test $st -eq 7

echo "[m2] SIGTERM forwarding (should exit)"
$BIN run --rootfs "$ROOTFS" -- /bin/sh -c 'sleep 1000' &
pid=$!
sleep 0.2
kill -TERM "$pid"
wait "$pid" || true

echo "[m2] zombie reaping (/proc state Z check)"
out=$($BIN run --rootfs "$ROOTFS" -- /bin/sh -c '
  sleep 0.05 & sleep 0.05 & sleep 0.05 &
  sleep 0.2
  for f in /proc/[0-9]*/stat; do
    st=$(cut -d" " -f3 "$f" 2>/dev/null || true)
    if [ "$st" = "Z" ]; then
      echo "FOUND ZOMBIE: $f"
      exit 2
    fi
  done
  echo OK
')
test "$out" = "OK"

echo "[m2] PASS"
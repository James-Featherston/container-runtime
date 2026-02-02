#!/usr/bin/env bash

set -euo pipefail # Exit on error, undefined variable, and pipefail if any commands fail

BIN=${1:-./minijfc} 
ROOTFS=${2:-./rootfs}

echo "[m1] UTS: hostname"

$BIN run --rootfs "$ROOTFS" --hostname mini -- /bin/sh -c 'hostname' | grep -qx "mini"

echo "[m1] rootfs: / contains bin and proc"
out=$($BIN run --rootfs "$ROOTFS" -- /bin/sh -c 'ls /')
echo "$out" | grep -q '^bin$'
echo "$out" | grep -q '^proc$'

echo "[m1] /proc mounted: /proc/self exists"
$BIN run --rootfs "$ROOTFS" -- /bin/sh -c 'test -e /proc/self'

echo "[m1] PID namespace: ps shows PID 1"
# BusyBox ps output varies, but PID 1 should appear somewhere.
$BIN run --rootfs "$ROOTFS" -- /bin/sh -c 'ps' | grep -E '(^|[[:space:]])1[[:space:]]' >/dev/null

echo "[m1] USER namespace: id reports uid=0"
$BIN run --rootfs "$ROOTFS" -- /bin/sh -c 'id -u' | grep -qx "0"

echo "[m1] PASS"

exit 0

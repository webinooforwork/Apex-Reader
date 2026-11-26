#!/data/data/com.termux/files/usr/bin/sh
set -eu
ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
JOBS=${APEX_JOBS:-2}
cmake -S "$ROOT" -B "$ROOT/build/termux" -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build "$ROOT/build/termux" --parallel "$JOBS"

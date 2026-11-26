#!/data/data/com.termux/files/usr/bin/sh
set -eu
ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
cmake -S "$ROOT" -B "$ROOT/build/test" -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build "$ROOT/build/test" --parallel "${APEX_JOBS:-2}"

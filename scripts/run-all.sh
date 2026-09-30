#!/usr/bin/env bash
# Build and run every tutorial demo.
#
# Usage:
#   ./scripts/run-all.sh                              # Aria 已安装时
#   ./scripts/run-all.sh --aria-root ~/Learning/Aria  # 用源码树
#
# Requires: CMake 3.20+ and a C++23 compiler (GCC 14+ / Clang 19+ / AppleClang 21+ / MSVC v143+).

set -euo pipefail

root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
build="$root/build"
aria_root=""

while [[ $# -gt 0 ]]; do
    case "$1" in
        --aria-root) aria_root="$2"; shift 2 ;;
        --build-dir) build="$2"; shift 2 ;;
        *) echo "unknown option: $1" >&2; exit 1 ;;
    esac
done

cmake_args=(-S "$root" -B "$build" -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON)
if [[ -n "$aria_root" ]]; then
    cmake_args+=("-DARIA_ROOT=$aria_root")
fi

echo "== configure =="
cmake "${cmake_args[@]}"

echo "== build =="
cmake --build "$build" --config Release --parallel

ctest --test-dir "$build" -C Release --output-on-failure --no-tests=error --timeout 30

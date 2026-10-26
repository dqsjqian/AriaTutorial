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

args=(--build-dir "$build" --test)
if [[ -n "$aria_root" ]]; then args+=(--aria-root "$aria_root"); fi
exec "${PYTHON:-python3}" "$root/scripts/build.py" "${args[@]}"

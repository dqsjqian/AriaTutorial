#!/usr/bin/env python3
"""Unified build pipeline for AriaTutorial (Windows/macOS/Linux).

Complete pipeline: deps -> build -> test -> bench -> package.
Powered by aria_deps.build_kit (pip install aria-deps).

Supports native, Qt and web targets.

Usage:
    python tools/build.py                           # Native release build
    python tools/build.py --platform qt             # Qt build
    python tools/build.py deps                      # Only dependencies
"""
from __future__ import annotations

import argparse
import platform as plat
import sys
from pathlib import Path

try:
    from aria_deps.build_kit import Pipeline
    _HAS_BUILD_KIT = True
except ImportError:
    _HAS_BUILD_KIT = False
    Pipeline = None

ROOT = Path(__file__).resolve().parents[1]


def extra_args(parser: argparse.ArgumentParser):
    parser.add_argument("--platform", choices=("native", "qt", "web"),
                        default="native", help="Target platform")
    parser.add_argument("--test", action="store_true", help="Run CTest after build")
    parser.add_argument("--offline", action="store_true", help="Offline dependency mode")
    parser.add_argument("--aria-root", type=Path, help="Explicit local Aria source")
    parser.add_argument("--aria-prefix", type=Path, help="Installed Aria SDK prefix")
    parser.add_argument("--all-demos", action="store_true", help="Build all demos")


def validate(args):
    if args.aria_root and args.aria_prefix:
        raise ValueError("--aria-root and --aria-prefix are mutually exclusive")


def build_dir_fn(args) -> Path:
    suffix = f"{args.platform}-{args.config.lower()}"
    if args.aria_prefix:
        suffix += "-sdk"
    return (ROOT / "build" / "unified" / suffix).resolve()


def deps_list(args) -> list:
    if args.aria_root or args.aria_prefix:
        return []
    fetch = [sys.executable, str(ROOT / "tools/ci/fetch_aria.py")]
    if args.offline:
        fetch.append("--offline")
    return [("aria", fetch)]


def cmake_flags(args) -> dict:
    flags = {"ARIA_DEPENDENCIES_OFFLINE": "ON" if args.offline else "OFF"}
    if args.aria_prefix:
        flags["ARIA_SDK_PREFIX"] = str(args.aria_prefix.resolve())
        flags["ARIA_ROOT"] = ""
    else:
        aria = args.aria_root.resolve() if args.aria_root else ROOT / "build/deps/aria"
        flags["ARIA_DIR"] = str(aria)
    # Tutorial-specific
    if args.all_demos:
        flags["ARIA_TUTORIAL_ALL_DEMOS"] = "ON"
    return flags


def main(argv=None) -> int:
    if not _HAS_BUILD_KIT:
        print("Error: aria-deps is required. Install it with:", file=sys.stderr)
        print("    pip install aria-deps", file=sys.stderr)
        return 1
    pipeline = Pipeline(
        name="aria-tutorial",
        root=ROOT,
        deps=deps_list,
        cmake_flags=cmake_flags,
        extra_args=extra_args,
        build_dir_fn=build_dir_fn,
        validate_fn=validate,
    )
    return pipeline.run(argv)


if __name__ == "__main__":
    sys.exit(main())

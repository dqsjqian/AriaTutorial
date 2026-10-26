# Build and run every tutorial demo.
#
# Usage:
#   .\scripts\run-all.ps1                              # Aria 已安装时
#   .\scripts\run-all.ps1 -AriaRoot D:\Learning\Aria   # 用源码树
#
# Requires: CMake 3.20+ and a C++23 compiler (MSVC v143+ / GCC 14+ / Clang 19+ / AppleClang 21+).
# On Windows run this from a Developer PowerShell for VS, or build Aria first.

param(
    [string]$AriaRoot = "",
    [string]$BuildDir = "build"
)

$ErrorActionPreference = "Stop"
$root = Split-Path -Parent $PSScriptRoot
$build = Join-Path $root $BuildDir

$buildArgs = @("$root/scripts/build.py", "--build-dir", $build, "--test")
if ($AriaRoot -ne "") { $buildArgs += @("--aria-root", $AriaRoot) }
& python @buildArgs
exit $LASTEXITCODE

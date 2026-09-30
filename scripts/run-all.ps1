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

$cmakeArgs = @("-S", $root, "-B", $build, "-DCMAKE_BUILD_TYPE=Release", "-DBUILD_TESTING=ON")
if ($AriaRoot -ne "") { $cmakeArgs += "-DARIA_ROOT=$AriaRoot" }

Write-Host "== configure =="
& cmake @cmakeArgs
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

Write-Host "== build =="
& cmake --build $build --config Release --parallel
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

& ctest --test-dir $build -C Release --output-on-failure --no-tests=error --timeout 30
exit $LASTEXITCODE

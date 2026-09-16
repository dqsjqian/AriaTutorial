# Build and run every tutorial demo.
#
# Usage:
#   .\scripts\run-all.ps1                              # Aria 已安装时
#   .\scripts\run-all.ps1 -AriaRoot D:\Learning\Aria   # 用源码树
#
# Requires: CMake 3.20+ and a C++20 compiler (MSVC v143+ / GCC 12+ / Clang 15+).
# On Windows run this from a Developer PowerShell for VS, or build Aria first.

param(
    [string]$AriaRoot = "",
    [string]$BuildDir = "build"
)

$ErrorActionPreference = "Stop"
$root = Split-Path -Parent $PSScriptRoot
$build = Join-Path $root $BuildDir

$cmakeArgs = @("-S", $root, "-B", $build, "-DCMAKE_BUILD_TYPE=Release")
if ($AriaRoot -ne "") { $cmakeArgs += "-DARIA_ROOT=$AriaRoot" }

Write-Host "== configure =="
& cmake @cmakeArgs
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

Write-Host "== build =="
& cmake --build $build --parallel
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

$bin = Join-Path $build "bin"
Get-ChildItem -Path $bin -Filter "ch*.exe" | Sort-Object Name | ForEach-Object {
    Write-Host ""
    Write-Host "===== $($_.Name) ====="
    & $_.FullName
}

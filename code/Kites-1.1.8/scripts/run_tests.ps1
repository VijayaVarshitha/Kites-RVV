# Builds and runs the whole Google Test suite (Windows, Qt 6.12 MinGW toolchain).
# Usage (from any folder):   powershell -ExecutionPolicy Bypass -File scripts\run_tests.ps1
# Only the vector tests:     powershell -ExecutionPolicy Bypass -File scripts\run_tests.ps1 -Filter "Rvv"
# The paths to Qt, MinGW and Ninja come from CMakePresets.json (C:/Qt/6.12.0/mingw_64,
# C:/Qt/Tools/mingw1310_64, C:/Qt/Tools/Ninja). Edit the "base" preset there if yours differ.
param([string]$Filter = "")
$ErrorActionPreference = "Stop"
Set-Location (Join-Path $PSScriptRoot "..")
$env:PATH = "C:/Qt/Tools/CMake_64/bin;C:/Qt/Tools/Ninja;C:/Qt/Tools/mingw1310_64/bin;C:/Qt/6.12.0/mingw_64/bin;" + $env:PATH

cmake --preset test
if ($LASTEXITCODE -ne 0) { throw "CMake configure failed" }
cmake --build --preset test-only --parallel
if ($LASTEXITCODE -ne 0) { throw "Build failed" }
if ($Filter -eq "") {
    ctest --preset test-only --output-on-failure
} else {
    ctest --preset test-only --output-on-failure -R $Filter
}
exit $LASTEXITCODE

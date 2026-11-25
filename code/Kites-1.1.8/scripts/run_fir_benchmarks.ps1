# Runs the five FIR example programs on the single-cycle processor and three pipelines
# with the command-line simulator, and prints instructions and cycles for each run.
# Usage:  powershell -ExecutionPolicy Bypass -File scripts\run_fir_benchmarks.ps1
# The full performance report of one run:  build\cli\kites-cli.exe examples\fir_vector_fp.s --vm rv5-h-f --stats
# (rv5-nh-nf is left out: without hazard detection or forwarding these programs need nops.)
$ErrorActionPreference = "Stop"
Set-Location (Join-Path $PSScriptRoot "..")
$env:PATH = "C:/Qt/Tools/CMake_64/bin;C:/Qt/Tools/Ninja;C:/Qt/Tools/mingw1310_64/bin;C:/Qt/6.12.0/mingw_64/bin;" + $env:PATH

cmake --preset cli
if ($LASTEXITCODE -ne 0) { throw "CMake configure failed" }
cmake --build --preset cli --parallel
if ($LASTEXITCODE -ne 0) { throw "Build failed" }
$cli = Get-ChildItem -Recurse -Path build\cli -Filter kites-cli.exe | Select-Object -First 1

$programs = "fir_scalar.s", "fir_vector.s", "fir_vector_macc.s", "fir_scalar_fp.s", "fir_vector_fp.s"
$vms = "rvss", "rv5-h-f", "rv5-h-nf", "rv5-nh-f"
"{0,-20} {1,-10} {2,14} {3,8}" -f "program", "vm", "instructions", "cycles"
foreach ($p in $programs) {
    foreach ($vm in $vms) {
        $out = & $cli.FullName ("examples\" + $p) --vm $vm --stats 2>&1 | Out-String
        $instr = [regex]::Match($out, "Instructions retired:\s*([\d,]+)").Groups[1].Value
        $cycles = [regex]::Match($out, "Cycles:\s*([\d,]+)").Groups[1].Value
        "{0,-20} {1,-10} {2,14} {3,8}" -f $p, $vm, $instr, $cycles
    }
}

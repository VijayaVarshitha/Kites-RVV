@echo off
rem ======================================================================
rem  Pipelined Vector Processing in KITES  -  guide for evaluators
rem  Double-click this file and choose an option from the menu.
rem ======================================================================
setlocal
title Kites RVV - Evaluator Guide
set "ROOT=%~dp0"
set "CODE=%ROOT%code\Kites-1.1.8"
set "TESTS=%CODE%\newly_added_testcases"
set "PATH=C:\Qt\Tools\CMake_64\bin;C:\Qt\Tools\Ninja;C:\Qt\Tools\mingw1310_64\bin;C:\Qt\6.12.0\mingw_64\bin;%PATH%"

:menu
cls
echo ======================================================================
echo   Pipelined Vector Processing in KITES  (RISC-V Vector extension)
echo ======================================================================
echo.
echo   Folders:  code\Kites-1.1.8   source code of our simulator
echo             presentation       our slides
echo             report             project report
echo.
echo   Needs Qt 6.12 (MinGW 13.1 kit), CMake and Ninja installed in C:\Qt
echo   (the default Qt installer paths).
echo.
echo   1. Open our simulator  (builds it the first time, a few minutes)
echo   2. Manual tests        (17 .s programs + expected results)
echo   3. Run our 1024 Google tests for the vector extension
echo   4. Run the original 158 Kites tests (to show nothing broke)
echo   5. Run the FIR benchmarks (scalar vs vector, instructions + cycles)
echo   6. Open the README
echo   7. Open the report and slides folders
echo   0. Exit
echo.
set "CH="
set /p "CH=Choose 0-7: "
if "%CH%"=="1" goto open_sim
if "%CH%"=="2" goto manual
if "%CH%"=="3" goto google
if "%CH%"=="4" goto original
if "%CH%"=="5" goto fir
if "%CH%"=="6" goto readme
if "%CH%"=="7" goto docs
if "%CH%"=="0" exit /b 0
goto menu

rem ----------------------------------------------------------------------
:open_sim
if not exist "%CODE%\KitesApp\Kites.exe" (
    echo.
    echo Building the simulator for the first time, please wait...
    call "%CODE%\scripts\build_app.bat" nopause
    if not exist "%CODE%\KitesApp\Kites.exe" goto fail
)
start "" "%CODE%\KitesApp\Kites.exe"
echo.
echo Kites is opening.
echo  - File - Open any .s file (examples are in code\Kites-1.1.8\examples)
echo  - Pick a processor (single cycle or one of the 5-stage pipelines)
echo  - Assemble, then Run or Step.
echo  - Vector registers v0-v31, VL and VTYPE are shown in the register panel.
pause
goto menu

rem ----------------------------------------------------------------------
:manual
cls
echo MANUAL TESTS  (folder: code\Kites-1.1.8\newly_added_testcases\manual_tests)
echo.
echo  test01  vsetvli (VL from AVL and SEW)     test09  pipeline hazard (scalar to vector)
echo  test02  vle32 / vse32                     test10  vfadd, vfsub, vfmul
echo  test03  e8 / e16 / e64 widths             test11  vfmacc, vfmv.v.f
echo  test04  vadd, vsub                        test12  NaN / infinity cases
echo  test05  vmul                              test13  e64 double precision
echo  test06  vmacc                             test14a/b  integer FIR scalar vs vector
echo  test07  vmv.v.x, vmv.v.v, vmv.v.i         test15a/b  float FIR scalar vs vector
echo  test08  tail undisturbed (past VL)
echo.
echo How to check one test:
echo  1. In Kites: File - Open - manual_tests\testNN_....s
echo  2. Choose the processor, Assemble, Run.
echo  3. Compare the registers with the Expected values in TEST_RESULTS.md
echo     (it has our results on all 5 processors).
echo.
echo Opening the folder, TEST_RESULTS.md and the simulator...
start "" explorer "%TESTS%\manual_tests"
start "" notepad "%TESTS%\manual_tests\TEST_RESULTS.md"
if exist "%CODE%\KitesApp\Kites.exe" (
    start "" "%CODE%\KitesApp\Kites.exe"
) else (
    echo The simulator is not built yet - choose option 1 first.
)
pause
goto menu

rem ----------------------------------------------------------------------
:google
cls
echo Building and running our 1024 Google tests (16 hand-written + 1008 generated).
echo The first build takes a few minutes; the run takes 1-2 minutes.
echo.
call "%TESTS%\google_tests\run_google_tests.bat"
goto menu

rem ----------------------------------------------------------------------
:original
cls
echo Building and running the original Kites tests...
echo.
pushd "%CODE%"
if not exist "build\test\CMakeCache.txt" (
    cmake --preset test || (popd & goto fail)
)
cmake --build --preset test-only --parallel || (popd & goto fail)
ctest --test-dir build\test -E Rvv --output-on-failure
popd
echo.
echo Expected: 100%% tests passed (158 original Kites tests).
pause
goto menu

rem ----------------------------------------------------------------------
:fir
cls
echo Building the command-line simulator and running the FIR programs...
echo.
powershell -NoProfile -ExecutionPolicy Bypass -File "%CODE%\scripts\run_fir_benchmarks.ps1"
echo.
echo Expected (instructions on single cycle / cycles on 5-stage hazard+forwarding):
echo   fir_scalar.s       1319 / 1704      fir_vector_macc.s   319 / 416
echo   fir_scalar_fp.s    1191 / 1576      fir_vector_fp.s     320 / 417
pause
goto menu

rem ----------------------------------------------------------------------
:readme
start "" notepad "%ROOT%README.txt"
start "" notepad "%TESTS%\README.txt"
goto menu

rem ----------------------------------------------------------------------
:docs
start "" explorer "%ROOT%report"
start "" explorer "%ROOT%presentation"
goto menu

rem ----------------------------------------------------------------------
:fail
echo.
echo Something failed - see the messages above.
echo Check that Qt 6.12 (MinGW), CMake and Ninja are installed under C:\Qt.
pause
goto menu

@echo off
rem Builds and runs our 1024 vector Google Tests. Double-click this file.
rem The full output is saved in vector_test_results.txt in this folder.
setlocal
set PATH=C:\Qt\Tools\CMake_64\bin;C:\Qt\Tools\Ninja;C:\Qt\Tools\mingw1310_64\bin;C:\Qt\6.12.0\mingw_64\bin;%PATH%
cd /d "%~dp0..\.."

if not exist "build\test\CMakeCache.txt" (
    echo === Configuring tests ===
    cmake --preset test || goto :fail
)
echo === Building kites_vector_tests ===
cmake --build build\test --target kites_vector_tests --parallel || goto :fail

set EXE=
for /r "build\test" %%F in (kites_vector_tests.exe) do if exist "%%F" set "EXE=%%F"
if "%EXE%"=="" (
    echo kites_vector_tests.exe was not found under build\test
    goto :fail
)

cd /d "%~dp0"
echo === Running 1024 tests (1-2 minutes) ===
"%EXE%" > vector_test_results.txt 2>&1
echo.
findstr /C:"tests from" /C:"PASSED" /C:"FAILED" vector_test_results.txt
echo.
echo Full output saved in %~dp0vector_test_results.txt
pause
exit /b 0

:fail
echo.
echo FAILED - send a screenshot of the error above.
pause
exit /b 1

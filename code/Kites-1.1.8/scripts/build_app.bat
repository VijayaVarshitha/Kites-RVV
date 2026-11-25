@echo off
rem Builds the Kites simulator (Release) and makes a ready-to-run copy in KitesApp\
rem next to the "code" folder, with all Qt DLLs, plus a "Kites" shortcut in that folder.
rem Usage: double-click this file (or "Kites.bat" in the top folder, which calls it once).
rem Qt paths: change QT below if Qt is not in C:\Qt\6.12.0\mingw_64.
setlocal
set QT=C:\Qt\6.12.0\mingw_64
set PATH=C:\Qt\Tools\CMake_64\bin;C:\Qt\Tools\Ninja;C:\Qt\Tools\mingw1310_64\bin;%QT%\bin;%PATH%
cd /d "%~dp0.."

echo === Configuring ===
cmake --preset release || goto :fail
echo === Building (takes a few minutes the first time) ===
cmake --build --preset release --parallel || goto :fail

set EXE=
for /r "build\release" %%F in (Kites.exe) do if exist "%%F" set "EXE=%%F"
if "%EXE%"=="" goto :fail

for %%A in ("%~dp0..") do set "ROOT=%%~fA"
set "APP=%ROOT%\KitesApp"
echo === Packaging into %APP% ===
if exist "%APP%" rmdir /s /q "%APP%"
mkdir "%APP%"
copy /y "%EXE%" "%APP%\Kites.exe" >nul || goto :fail
"%QT%\bin\windeployqt.exe" --release --compiler-runtime --no-translations "%APP%\Kites.exe" || goto :fail
xcopy /e /i /y /q examples "%APP%\examples" >nul

rem Shortcut "Kites" in the top folder. explorer.exe with a relative path keeps it working
rem when the folder is moved or unzipped somewhere else.
powershell -NoProfile -Command "$s=(New-Object -ComObject WScript.Shell).CreateShortcut('%ROOT%\Kites.lnk'); $s.TargetPath=\"$env:WINDIR\explorer.exe\"; $s.Arguments='KitesApp\Kites.exe'; $s.WorkingDirectory=''; $s.IconLocation='%APP%\Kites.exe,0'; $s.Description='Kites RISC-V simulator with RVV vector extension'; $s.Save()"

echo.
echo Done. Open the simulator with the "Kites" shortcut in %ROOT%
if /i not "%~1"=="nopause" pause
exit /b 0

:fail
echo.
echo BUILD FAILED - copy the error above and send it.
pause
exit /b 1

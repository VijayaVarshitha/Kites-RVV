@echo off
rem Opens our modified Kites simulator (RVV vector extension).
if not exist "%~dp0code\Kites-1.1.8\KitesApp\Kites.exe" (
    echo Kites is not built yet. Building it now, please wait...
    call "%~dp0code\Kites-1.1.8\scripts\build_app.bat" nopause
    if errorlevel 1 exit /b 1
)
start "" "%~dp0code\Kites-1.1.8\KitesApp\Kites.exe"

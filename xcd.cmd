@echo off
rem xcd.cmd - Windows wrapper for xcd-win.exe
rem Assumes xcd-win.exe is on the PATH and writes %TEMP%\xcd_target.txt.

set "TEMPFILE=%TEMP%\xcd_target.txt"

rem Remove any stale temp file from a previous run
if exist "%TEMPFILE%" del "%TEMPFILE%" >nul 2>&1

rem Call the core with all arguments
xcd-win %*
if errorlevel 1 (
    rem Core reported an error; do not change directories
    goto :eof
)

if not exist "%TEMPFILE%" (
    rem Core did not produce a target file; nothing to do
    goto :eof
)

set "TARGET_DIR="
set /p TARGET_DIR=<"%TEMPFILE%"
del "%TEMPFILE%" >nul 2>&1

if "%TARGET_DIR%"=="" (
    rem Empty target; nothing to do
    goto :eof
)

cd /d "%TARGET_DIR%"

:eof


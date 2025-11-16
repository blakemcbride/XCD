@echo off
setlocal ENABLEEXTENSIONS

rem xcd.cmd - Windows wrapper for xcd-win.exe
rem Assumes xcd-win.exe is on the PATH and writes %TEMP%\xcd_target.txt.

set "TEMPFILE=%TEMP%\xcd_target.txt"

rem Remove any stale temp file from a previous run
if exist "%TEMPFILE%" del "%TEMPFILE%" >nul 2>&1

rem Call the core with all arguments
xcd-win %*
set "ERR=%ERRORLEVEL%"

if not "%ERR%"=="0" (
    rem Core reported an error; do not change directories
    endlocal & exit /b %ERR%
)

if not exist "%TEMPFILE%" (
    rem Core did not produce a target file; nothing to do
    endlocal & exit /b 1
)

set "TARGET_DIR="
set /p TARGET_DIR=<"%TEMPFILE%"
del "%TEMPFILE%" >nul 2>&1

if "%TARGET_DIR%"=="" (
    rem Empty target; nothing to do
    endlocal & exit /b 1
)

cd /d "%TARGET_DIR%"

endlocal


@echo off
setlocal

rem Run the helper program
"%USERPROFILE%\xcd.exe" %*

if errorlevel 1 (
    exit /b %errorlevel%
)

rem Read the temp file written by xcd.exe
set TEMPFILE=%TEMP%\xcd_target.txt
if not exist "%TEMPFILE%" exit /b 1

set /p TARGET_DIR=<"%TEMPFILE%"

rem Change directory in the *current shell*
cd /d "%TARGET_DIR%"

del "%TEMPFILE%" >nul 2>&1

endlocal


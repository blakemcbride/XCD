# 🌀 xcd – A Smarter cd Command for Linux and Windows

xcd is an enhanced replacement for the standard cd command.
On Linux it is implemented as a bash function, and on Windows it is implemented as a combination of a helper program (xcd.exe) and a lightweight command wrapper (xcd.cmd).

Across both platforms, xcd remembers directories you visit, supports fuzzy navigation by directory name, and provides cycle previews, listing, and automatic memory pruning.
It makes moving around large projects or multi-directory environments dramatically faster.

## 🌐 Features (Both Platforms)
✔ Smart directory memory

Every successful xcd invocation stores a full absolute path in:

Linux: ~/.xcd_memory

Windows: %USERPROFILE%\.xcd_memory

Dead directories are automatically pruned.

✔ Fuzzy navigation
xcd Backend


finds directories whose final path component contains “Backend”.

If multiple matches exist:

The first xcd Backend jumps to the first match

Repeating xcd Backend cycles to the next

It wraps around when it reaches the end

✔ Directory cycling
xcd proj    # jumps to match #1
xcd proj    # jumps to match #2
xcd proj    # back to match #1

✔ Listing and preview options
Command	Purpose
xcd -l	List all remembered directories
xcd -l segment	List directories with basename matching segment
xcd -p segment	Show what matches exist and which will be chosen next
xcd -c	Clear the memory file
xcd -h	Help message

## 🐧 Linux Version (Bash)

The Linux version is a pure bash function that must be added to your ~/.bashrc.

It supports:

cd-compatible behavior

Fuzzy matching

Cycling through matches

-h, -l, -c, -p options

Home-directory normalization

Automatic pruning

Persistent directory memory

See the code in xcd.bash or your .bashrc as appropriate.

## 🪟 Windows Version (C Program + CMD Wrapper)

Because Windows executables cannot change the parent shell’s working directory,
the Windows version uses a two-part model:

xcd.exe

Written in C

Computes the target directory

Updates the persistent memory file

Writes the chosen directory to a temp file:

%TEMP%\xcd_target.txt


xcd.cmd

A wrapper that:

Runs xcd.exe

Reads the temp file

Changes the directory in the current CMD session

Deletes the temp file

This pattern matches the approach used by popular directory jumpers like zoxide, fasd, and fzf.

## 📦 Installing the Windows Version
1. Build or download xcd.exe

Compile with Visual Studio:

cl /EHsc xcd.c


Or MinGW:

gcc -o xcd.exe xcd.c


Copy xcd.exe somewhere on your PATH, such as:

C:\Users\<you>\bin\xcd.exe

2. Create xcd.cmd

Place this file in the same directory as xcd.exe and ensure that directory is on your PATH.

xcd.cmd:

@echo off
setlocal

"%USERPROFILE%\bin\xcd.exe" %*

if errorlevel 1 (
    exit /b %errorlevel%
)

set TEMPFILE=%TEMP%\xcd_target.txt
if not exist "%TEMPFILE%" exit /b 1

set /p TARGET_DIR=<"%TEMPFILE%"
cd /d "%TARGET_DIR%"

del "%TEMPFILE%" >nul 2>&1
endlocal


Now you can use:

xcd Backend
xcd -l
xcd -p Api
xcd -c


just like on Linux.

## 🧠 How Windows Memory Works

Memory file (same concept as Linux):

%USERPROFILE%\.xcd_memory


Each line stores an absolute directory path.

xcd.exe:

loads this file

prunes dead entries

performs fuzzy matching

chooses the target directory

writes the final choice to %TEMP%\xcd_target.txt

xcd.cmd then performs the actual directory change.

## 🤝 Authors & Attribution
Primary Author

Blake McBride
Creator, maintainer, and architect of xcd on both Linux and Windows.

Assisted by

ChatGPT (OpenAI) — used as a development tool for code generation, planning, debugging, and documentation assistance.
All final code decisions and designs were made by the project author.


# 🌀 xcd – A Smarter cd Command for Linux and Windows

xcd is an enhanced replacement for the standard cd command.
On Linux it is implemented as a bash function, and on Windows it is implemented as a combination of a helper program (xcd.exe) and a lightweight command wrapper (xcd.cmd).

Across both platforms, xcd remembers directories you visit, supports fuzzy navigation by directory name, and provides cycle previews, listing, and automatic memory pruning.

It makes navigating large projects, monorepos, multi-directory environments, and development trees dramatically faster.

## 🌐 Features (Both Platforms)
✔ Smart directory memory

Every successful xcd stores a full absolute path in a memory file:

Linux: ~/.xcd_memory

Windows: %USERPROFILE%\.xcd_memory

Dead directories are auto-removed.

✔ Fuzzy navigation

Running:

xcd Backend


finds directories whose final path component contains the string “Backend”.

If multiple matches exist:

First call goes to the first match

Next call goes to the second

Continues cycling with wrap-around

✔ Listing and preview options
Command	Purpose
xcd -l	List all remembered directories
xcd -l segment	List directories containing “segment” in the basename
xcd -p segment	Preview matches and see which directory xcd segment will jump to next
xcd -c	Clear the memory file
xcd -h	Help message describing behavior
🐧 Linux Version (Bash Function)

The Linux version is a pure Bash function that runs inside your shell, which allows it to actually change directories (scripts cannot).
It supports:

Argument handling identical to cd

Fuzzy matching and cycling

-l, -p, -c, -h

Auto-pruning of memory entries

Symlink-aware home handling (e.g., /home/blake -> /drive1/...)

Persistent memory stored in ~/.xcd_memory

## 📦 Installing on Linux
1. Add xcd to your shell startup

Copy the full xcd function into your ~/.bashrc.
For example:

nano ~/.bashrc


Paste the entire function.

Then reload your shell:

source ~/.bashrc


Or open a new terminal.

2. Optional: put it in a separate file

If you want a cleaner ~/.bashrc:

mkdir -p ~/.local/share/xcd
cp xcd.bash ~/.local/share/xcd/xcd.bash


Then add to ~/.bashrc:

source ~/.local/share/xcd/xcd.bash

3. Verify installation

Run:

xcd -h


You should see the full help message.

Try jumping around:

xcd /etc
xcd
xcd -l


Try fuzzy navigation:

xcd etc


Try cycling:

xcd src
xcd src
xcd src

4. Removing / resetting

At any time:

xcd -c


clears ~/.xcd_memory completely.

Or remove the function from ~/.bashrc.

## 🪟 Windows Version (C Program + CMD Wrapper)

Because Windows executables cannot change the current shell directory,
xcd is implemented as:

xcd.exe

Computes the target directory

Updates memory

Writes the chosen directory into a temporary file

xcd.cmd

Runs xcd.exe

Reads the temporary file

Performs the actual cd

Deletes the temporary file

This approach is identical to how tools like fzf, autojump, and zoxide work on Windows.

### 📦 Installing on Windows
1. Build or download xcd.exe

Visual Studio:

cl /EHsc xcd.c


MinGW:

gcc -o xcd.exe xcd.c


Place xcd.exe somewhere on your PATH—for example:

C:\Users\<you>\bin\xcd.exe

2. Create xcd.cmd

Place this file in the same directory as xcd.exe:

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


Now you can:

xcd Backend
xcd -l
xcd -p Core
xcd -c


just like on Linux.

## 🧠 How Memory Works (Both Platforms)

The memory file is:

simple plain text

one directory per line

automatically deduplicated

automatically pruned when a directory vanishes

Linux path:

~/.xcd_memory


Windows path:

%USERPROFILE%\.xcd_memory


You can edit or delete it manually if desired.

## 🤝 Authors & Attribution
Primary Author

Blake McBride
Creator, maintainer, and architect of the xcd project on both Linux and Windows.

Assisted by

ChatGPT (OpenAI) — used as a development and documentation tool.
All final design decisions and modifications were made by the project author.


# xcd - A cross-platform, fuzzy directory jumper for **Linux**, **macOS**, and **Windows**

`xcd` is a smart replacement for `cd` that remembers directories you visit and lets you jump back to them using short “segments” of their name. It behaves like a more intuitive, memory-based `cd`.

Examples:

```bash
xcd Backend         # jumps to a remembered directory whose basename contains "Backend"
xcd api             # cycles through directories whose basename contains "api"
xcd -l              # lists all remembered directories
xcd -p Backend      # previews which directory "xcd Backend" would choose next
```

`xcd` automatically:

- Remembers every directory you **start in**
- Remembers every directory you **jump to**
- Stores only **canonical, unique** paths  
- Lets you fuzzy-match on the final path segment  
- Cycles through matches intelligently  
- Works uniformly across Linux, macOS, and Windows  

Internally, `xcd` has two components:

- A **C core** that contains all matching/logic  
  - `xcd-core` on Linux/macOS  
  - `xcd-win.exe` on Windows  
- A **thin wrapper**  
  - `xcd.sh` on Linux/macOS  
  - `xcd.cmd` on Windows  

Users type only `xcd`. The wrapper handles `cd` and the core handles everything else.

All wrapper files are provided in the repository.

---

# Features

### ✔ Drop-in replacement for `cd`
Just type `xcd` exactly as you would `cd`, with extra powers.

### ✔ Fuzzy directory matching
```bash
xcd Backend
xcd logs
xcd api
```
Matches directories whose **basename** contains the given segment.

### ✔ Cycling between matches
If multiple directories match a segment, repeated `xcd segment` cycles through them.

### ✔ Canonical path storage
All paths are normalized via:

- `realpath()` on Linux/macOS  
- `_fullpath()` on Windows  

Ensuring uniqueness and predictable behavior.

### ✔ Stores two directories per navigation:
1. The directory in which you *ran* `xcd`
2. The directory that `xcd` *takes you to*

### ✔ Management commands
```
xcd -l           # list all remembered directories
xcd -l segment   # list only matches
xcd -p segment   # preview what 'xcd segment' would do next
xcd -c           # clear memory
xcd -h           # help
```

### ✔ Cross-platform
- Linux (Bash)
- macOS (zsh or Bash)
- Windows (CMD)

---

# Memory File

All platforms use:

```
~/.xcd_memory
```

which contains canonical absolute paths like:

```
/home/XXXX/projects/foo
/home/XXXX/src/backend
/drive1/ROOT/home/XXXX/Stack360
C:\Users\XXXX\Documents\XCD
```

Duplicates are avoided automatically.

---

# Installation

# Linux / macOS

## 1. Build the C core

```bash
gcc -std=c11 -Wall -O2 -o xcd-core xcd-core.c
```

Place it on your PATH:

```bash
mkdir -p ~/.local/bin
mv xcd-core ~/.local/bin/
```

Ensure `~/.local/bin` is on your PATH.

---

## 2. Source the provided wrapper (`xcd.sh`)

In `~/.bashrc` (Linux) or `~/.zshrc` (macOS):

```bash
source /full/path/to/xcd.sh
```

Reload:

```bash
source ~/.bashrc    # or ~/.zshrc
```

### macOS notes
- macOS already includes `/bin/bash`; **you do NOT need to install Bash**.
- The included `xcd.sh` wrapper is **POSIX-compatible**, so it works when sourced from `zsh`.

---

# Windows

Windows uses:

- `xcd-win.exe` (compiled from `xcd-win.c`)
- `xcd.cmd` (wrapper that performs the actual directory change)

Users type only:

```
xcd
```

---

## 1. Build the Windows C core

Using MSVC:

```cmd
cl /EHsc xcd-win.c /Fe:xcd-win.exe
```

Using MinGW:

```cmd
gcc -Wall -O2 -o xcd-win.exe xcd-win.c
```

---

## 2. Install `xcd-win.exe` and `xcd.cmd`

```cmd
mkdir %USERPROFILE%\bin
copy xcd-win.exe %USERPROFILE%\bin\
copy xcd.cmd %USERPROFILE%\bin\
```

Ensure `%USERPROFILE%\bin` is on your PATH.

Now commands like:

```cmd
xcd Backend
xcd -l
xcd -p Backend
xcd -c
```

work exactly like Linux.

### How it works

1. `xcd.cmd` runs  
2. Calls `xcd-win.exe`  
3. Core writes target directory to `%TEMP%\xcd_target.txt`  
4. Wrapper reads file  
5. Wrapper performs `cd /d <target>`  
6. Deletes the temp file  

---

# Attribution

**Primary author:**  
**Blake McBride**  
- Concept, design, directory memory behavior, matching rules, wrappers, and cross‑platform integration.

**AI-assisted implementation:**  
**ChatGPT (OpenAI)**  
- Help generating and refining core logic, wrappers, documentation, and debugging.

All final decisions, integration, and architecture are by Blake McBride.

---

# License

Choose any license you prefer — MIT, BSD, Apache, GPL, Unlicense, etc.


🌀 xcd – A Smarter cd Command for Bash

xcd is a feature-enhanced replacement for the standard cd command in GNU Bash.
It remembers directories you’ve visited, supports intelligent fuzzy navigation by name,
and adds convenient listing, cycling, preview, and maintenance features — all without leaving your shell.

This tool is ideal for developers working across many directories, large codebases, or multi-repo environments.

✨ Features
✔ Smart directory memory

Every successful call to xcd stores the absolute physical path of the directory in ~/.xcd_memory.
Entries are automatically de-duplicated and non-existent paths are pruned.

✔ Fuzzy navigation

If you run:

xcd Backend


and ./Backend does not exist, xcd will:

Search the memory file for directories whose final path component contains "Backend"

Jump to the first match

Cycle through subsequent matches each time you run the same command

✔ Directory cycling

Repeated calls automatically rotate among all matching directories:

xcd proj   # goes to match 1
xcd proj   # goes to match 2
xcd proj   # wraps back to match 1

✔ Listing & preview options
Command	Description
xcd -l	List all remembered directories
xcd -l name	List only entries whose final component contains name
xcd -p name	Preview the match cycle for name without changing directories
✔ Maintenance options
Command	Description
xcd -c	Clear the memory file
xcd -h	Display full help
✔ Symlink-safe home handling

If your home directory is a symlink (e.g. /home/blake → /drive1/ROOT/home/blake), xcd automatically normalizes paths so your shell prompt still displays ~ correctly.

📦 Installation

Add the following line to your ~/.bashrc:

source /path/to/xcd.sh


Or copy the function directly into your ~/.bashrc.

Reload your shell:

source ~/.bashrc


You’re ready to use xcd.

🧠 Usage Examples
Jump to a directory and remember it
xcd ~/projects/myapp

Fuzzy navigation
xcd app       # jumps to the first directory whose basename contains "app"

Cycle through matches
xcd app
xcd app

Preview before jumping
xcd -p Backend

List all remembered directories
xcd -l

📁 Memory File

xcd stores its persistent directory memory here:

~/.xcd_memory


This file is:

plain text

one directory per line

auto-cleaned on every invocation

safe to edit or delete manually

🧑‍💻 Authors & Attribution
Primary Author

Blake McBride
Creator, maintainer, and owner of the project logic, design, and implementation.

Assistance

This project includes portions of code and design guidance generated with the assistance of
ChatGPT (OpenAI), used as a tool for code-generation and refinement.
All final decisions and modifications were performed by the primary author.



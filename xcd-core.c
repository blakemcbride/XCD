// xcd-core.c - cross-platform (Linux/macOS) core for xcd
// Compile with:  gcc -std=c11 -Wall -O2 -o xcd-core xcd-core.c

#define _GNU_SOURCE
#define _POSIX_C_SOURCE 200809L
#define _DARWIN_C_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <unistd.h>
#include <sys/stat.h>
#include <pwd.h>
#include <errno.h>

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

#define MAX_DIRS 8192

static char *dirs[MAX_DIRS];
static int dir_count = 0;
static int memory_dirty = 0;
static char memory_file[PATH_MAX];

/* ---------- Utilities ---------- */

static const char *get_home()
{
    const char *home = getenv("HOME");
    if (home && *home)
        return home;

    struct passwd *pw = getpwuid(getuid());
    if (pw && pw->pw_dir && *pw->pw_dir)
        return pw->pw_dir;

    fprintf(stderr, "xcd-core: cannot determine HOME\n");
    exit(1);
}

static int is_dir(const char *path)
{
    struct stat st;
    if (stat(path, &st) != 0)
        return 0;
    return S_ISDIR(st.st_mode);
}

static char *canonical_path(const char *path, char *buf, size_t buflen)
{
    if (!realpath(path, buf))
        return NULL;
    buf[buflen - 1] = '\0';
    return buf;
}

static void free_memory_list()
{
    for (int i = 0; i < dir_count; i++)
        free(dirs[i]);
    dir_count = 0;
}

static int contains_dir(const char *path)
{
    for (int i = 0; i < dir_count; i++)
        if (strcmp(dirs[i], path) == 0)
            return 1;
    return 0;
}

/* ---------- Load / Save memory ---------- */

static void load_memory()
{
    const char *home = get_home();
    snprintf(memory_file, sizeof(memory_file), "%s/.xcd_memory", home);
    memory_file[sizeof(memory_file) - 1] = '\0';

    FILE *f = fopen(memory_file, "r");
    if (!f)
        return; // no file yet, that's fine

    char line[PATH_MAX];

    while (dir_count < MAX_DIRS && fgets(line, sizeof(line), f))
    {
        char *nl = strchr(line, '\n');
        if (nl)
            *nl = '\0';

        if (line[0] == '\0')
            continue;

        if (!is_dir(line))
            continue;

        char canon[PATH_MAX];
        if (!canonical_path(line, canon, sizeof(canon)))
            continue;

        if (!contains_dir(canon))
        {
            dirs[dir_count] = strdup(canon);
            if (!dirs[dir_count])
            {
                fprintf(stderr, "xcd-core: out of memory\n");
                fclose(f);
                exit(1);
            }
            dir_count++;
        }
    }

    fclose(f);
}

static void save_memory()
{
    if (!memory_dirty)
        return;

    FILE *f = fopen(memory_file, "w");
    if (!f)
    {
        fprintf(stderr, "xcd-core: cannot write %s: %s\n",
                memory_file, strerror(errno));
        return;
    }

    for (int i = 0; i < dir_count; i++)
        fprintf(f, "%s\n", dirs[i]);

    fclose(f);
    memory_dirty = 0;
}

static void remember_dir(const char *path)
{
    if (!is_dir(path))
        return;

    char canon[PATH_MAX];
    if (!canonical_path(path, canon, sizeof(canon)))
        return;

    if (contains_dir(canon))
        return;

    if (dir_count >= MAX_DIRS)
        return;

    dirs[dir_count] = strdup(canon);
    if (!dirs[dir_count])
    {
        fprintf(stderr, "xcd-core: out of memory\n");
        exit(1);
    }

    dir_count++;
    memory_dirty = 1;
}

/* ---------- Commands ---------- */

static void cmd_help()
{
    printf(
        "xcd-core - core logic for xcd (Linux/macOS)\n\n"
        "Usage (called via shell wrapper):\n"
        "  xcd-core                 Print canonical HOME directory.\n"
        "  xcd-core DIR             Print canonical DIR if it exists.\n"
        "  xcd-core SEGMENT         Fuzzy match remembered dirs by basename and\n"
        "                           print the chosen target directory.\n"
        "\n"
        "Options (management / info; do NOT change directory):\n"
        "  xcd-core -h              Show this help.\n"
        "  xcd-core -l              List all remembered directories.\n"
        "  xcd-core -l SEGMENT      List remembered dirs whose basename contains SEGMENT.\n"
        "  xcd-core -p SEGMENT      Preview matches and which one would be used next.\n"
        "  xcd-core -c              Clear the memory file (~/.xcd_memory).\n"
        "\n"
        "Note: wrappers should only 'cd' into the directory printed when\n"
        "no -h/-l/-p/-c option is used.\n"
    );
}

static void cmd_clear()
{
    // Clear in-memory list and truncate file
    free_memory_list();
    FILE *f = fopen(memory_file, "w");
    if (f)
        fclose(f);
}

static void cmd_list(const char *segment)
{
    if (!segment || segment[0] == '\0')
    {
        // list all
        for (int i = 0; i < dir_count; i++)
            printf("%s\n", dirs[i]);
    }
    else
    {
        for (int i = 0; i < dir_count; i++)
        {
            const char *base = strrchr(dirs[i], '/');
            if (!base)
                base = dirs[i];
            else
                base++; // skip '/'

            if (strstr(base, segment))
                printf("%s\n", dirs[i]);
        }
    }
}

static int find_matches(const char *segment, int *out_indices, int max_indices)
{
    int count = 0;

    for (int i = 0; i < dir_count; i++)
    {
        const char *base = strrchr(dirs[i], '/');
        if (!base)
            base = dirs[i];
        else
            base++;

        if (strstr(base, segment))
        {
            if (count < max_indices)
                out_indices[count] = i;
            count++;
        }
    }

    return count;
}

static void cmd_preview(const char *segment)
{
    if (!segment || segment[0] == '\0')
    {
        fprintf(stderr, "xcd-core: -p requires a segment\n");
        return;
    }

    int indices[MAX_DIRS];
    int count = find_matches(segment, indices, MAX_DIRS);

    if (count == 0)
    {
        printf("No matches for \"%s\".\n", segment);
        return;
    }

    char cwd[PATH_MAX];
    if (!canonical_path(".", cwd, sizeof(cwd)))
    {
        fprintf(stderr, "xcd-core: cannot determine current directory\n");
        return;
    }

    int cur_idx = -1;

    for (int i = 0; i < count; i++)
    {
        if (strcmp(dirs[indices[i]], cwd) == 0)
        {
            cur_idx = i;
            break;
        }
    }

    int next = (cur_idx < 0) ? 0 : (cur_idx + 1) % count;

    printf("Matches for \"%s\":\n", segment);
    for (int i = 0; i < count; i++)
    {
        const char *mark = (i == cur_idx) ? "*" : " ";
        printf("  [%d]%s %s\n", i, mark, dirs[indices[i]]);
    }

    if (cur_idx >= 0)
        printf("Current directory is at index [%d].\n", cur_idx);
    else
        printf("Current directory is not in the match list.\n");

    printf("Next target for segment \"%s\": [%d] %s\n",
           segment, next, dirs[indices[next]]);
}

/* ---------- Navigation core ---------- */

static int cmd_navigate(int argc, char **argv)
{
    const char *home = get_home();

    if (argc == 0)
    {
        // No args: go to HOME
        char canon[PATH_MAX];
        if (!canonical_path(home, canon, sizeof(canon)))
        {
            // fallback: use HOME as-is
            strncpy(canon, home, sizeof(canon));
            canon[sizeof(canon) - 1] = '\0';
        }

        printf("%s\n", canon);
        remember_dir(canon);
        return 0;
    }

    const char *arg = argv[0];

    // If arg is an existing directory (absolute or relative), use it directly
    if (is_dir(arg))
    {
        char canon[PATH_MAX];
        if (!canonical_path(arg, canon, sizeof(canon)))
        {
            fprintf(stderr, "xcd-core: cannot resolve path: %s\n", arg);
            return 1;
        }

        printf("%s\n", canon);
        remember_dir(canon);
        return 0;
    }

    // If arg is relative and does not exist AND has no slash, do fuzzy search
    if (strchr(arg, '/') == NULL)
    {
        int indices[MAX_DIRS];
        int count = find_matches(arg, indices, MAX_DIRS);

        if (count == 0)
        {
            fprintf(stderr, "xcd-core: no directory matches \"%s\"\n", arg);
            return 1;
        }

        char cwd[PATH_MAX];
        if (!canonical_path(".", cwd, sizeof(cwd)))
        {
            fprintf(stderr, "xcd-core: cannot determine current directory\n");
            return 1;
        }

        int cur_idx = -1;

        for (int i = 0; i < count; i++)
        {
            if (strcmp(dirs[indices[i]], cwd) == 0)
            {
                cur_idx = i;
                break;
            }
        }

        int next = (cur_idx < 0) ? 0 : (cur_idx + 1) % count;
        const char *target = dirs[indices[next]];

        printf("%s\n", target);
        remember_dir(target); // may already be present; harmless
        return 0;
    }

    // Otherwise, treat as path that doesn't exist or is invalid
    fprintf(stderr, "xcd-core: \"%s\" is not a directory and not a simple segment\n", arg);
    return 1;
}

/* ---------- main ---------- */

int main(int argc, char **argv)
{
    load_memory();

    /* Management / info commands do NOT change dirs, and we won't
       add the current dir for those (so 'xcd-core -c' really clears). */

    if (argc >= 2)
    {
        const char *arg1 = argv[1];

        if (strcmp(arg1, "-h") == 0)
        {
            cmd_help();
            return 0;
        }

        if (strcmp(arg1, "-c") == 0)
        {
            cmd_clear();
            return 0;
        }

        if (strcmp(arg1, "-l") == 0)
        {
            const char *segment = (argc >= 3) ? argv[2] : NULL;
            cmd_list(segment);
            return 0;
        }

        if (strcmp(arg1, "-p") == 0)
        {
            const char *segment = (argc >= 3) ? argv[2] : NULL;
            cmd_preview(segment);
            return 0;
        }
    }

    /* Navigation mode:
       - First remember the directory we are currently in
       - Then compute the target directory and remember that too
    */
    char cwd[PATH_MAX];
    if (canonical_path(".", cwd, sizeof(cwd)))
        remember_dir(cwd);

    int rc;

    if (argc == 1)
    {
        // No args: go to HOME
        rc = cmd_navigate(0, NULL);
    }
    else
    {
        rc = cmd_navigate(argc - 1, &argv[1]);
    }

    if (memory_dirty)
        save_memory();

    return rc;
}


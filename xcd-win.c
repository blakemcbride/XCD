// xcd-win.c - Windows core for xcd
// Compile with (MSVC):  cl /EHsc xcd-win.c
// Or with MinGW:       gcc -Wall -O2 -o xcd-win.exe xcd.c

#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <direct.h>
#include <stdlib.h>

#define MAXDIRS  4096
#define MAXPATH  4096

static char *memory[MAXDIRS];
static int   memcount = 0;

static char memory_file[MAXPATH];
static char temp_file[MAXPATH];

/* ---------- Utilities ---------- */

static int is_dir(const char *path)
{
    DWORD attr = GetFileAttributesA(path);
    if (attr == INVALID_FILE_ATTRIBUTES)
        return 0;
    return (attr & FILE_ATTRIBUTE_DIRECTORY) != 0;
}

static int contains_dir(const char *path)
{
    for (int i = 0; i < memcount; i++)
        if (_stricmp(memory[i], path) == 0)
            return 1;
    return 0;
}

/* ---------- Memory file handling ---------- */

static void load_memory(void)
{
    FILE *f = fopen(memory_file, "r");
    if (!f)
        return;

    char buf[MAXPATH];

    while (memcount < MAXDIRS && fgets(buf, sizeof(buf), f))
    {
        char *nl = strchr(buf, '\n');
        if (nl)
            *nl = '\0';

        if (buf[0] == '\0')
            continue;

        if (!is_dir(buf))
            continue;

        if (!contains_dir(buf))
        {
            memory[memcount] = _strdup(buf);
            if (!memory[memcount])
            {
                fprintf(stderr, "xcd: out of memory\n");
                fclose(f);
                exit(1);
            }
            memcount++;
        }
    }

    fclose(f);
}

static void save_memory(void)
{
    FILE *f = fopen(memory_file, "w");
    if (!f)
    {
        fprintf(stderr, "xcd: cannot write %s\n", memory_file);
        return;
    }

    for (int i = 0; i < memcount; i++)
        fprintf(f, "%s\n", memory[i]);

    fclose(f);
}

static void remember_dir(const char *path)
{
    if (!is_dir(path))
        return;

    char full[MAXPATH];

    if (!_fullpath(full, path, MAXPATH))
        return;

    if (contains_dir(full))
        return;

    if (memcount >= MAXDIRS)
        return;

    memory[memcount] = _strdup(full);
    if (!memory[memcount])
    {
        fprintf(stderr, "xcd: out of memory\n");
        exit(1);
    }

    memcount++;
    save_memory();
}

/* ---------- Temp file handling ---------- */

static void write_target(const char *dir)
{
    FILE *f = fopen(temp_file, "w");
    if (!f)
        return;

    fprintf(f, "%s", dir);
    fclose(f);
}

/* ---------- Info / UI ---------- */

static void print_help(void)
{
    printf("xcd - Windows directory jumper core\n\n");
    printf("Usage (called from xcd.cmd):\n");
    printf("  xcd                Go to home directory.\n");
    printf("  xcd <segment>      Fuzzy match on remembered directories by basename.\n");
    printf("  xcd <path>         Go directly to <path> if it exists.\n\n");
    printf("Options (do not change directory by themselves):\n");
    printf("  xcd -h             Show this help.\n");
    printf("  xcd -l             List all remembered directories.\n");
    printf("  xcd -l segment     List remembered dirs with basename containing segment.\n");
    printf("  xcd -p segment     Preview matches and their order.\n");
    printf("  xcd -c             Clear the memory file.\n\n");
    printf("Note:\n");
    printf("  The shell wrapper (xcd.cmd) uses the directory written to\n");
    printf("  %%TEMP%%\\xcd_target.txt to actually change the current directory.\n");
}

/* ---------- Matching ---------- */

static int find_matches(const char *seg, int *out_indices, int max)
{
    int count = 0;

    for (int i = 0; i < memcount; i++)
    {
        const char *base = strrchr(memory[i], '\\');
        if (!base)
            base = memory[i];
        else
            base++;

        if (strstr(base, seg) != NULL)
        {
            if (count < max)
                out_indices[count] = i;
            count++;
        }
    }

    return count;
}

/* ---------- main ---------- */

int main(int argc, char **argv)
{
    const char *home = getenv("USERPROFILE");
    if (!home)
    {
        fprintf(stderr, "xcd: USERPROFILE not set\n");
        return 1;
    }

    /* Build paths for memory file and temp file */
    snprintf(memory_file, sizeof(memory_file), "%s\\.xcd_memory", home);
    memory_file[sizeof(memory_file) - 1] = '\0';

    char tmp[MAXPATH];
    DWORD tmp_len = GetTempPathA(sizeof(tmp), tmp);
    if (tmp_len == 0 || tmp_len > sizeof(tmp))
    {
        fprintf(stderr, "xcd: GetTempPath failed\n");
        return 1;
    }

    snprintf(temp_file, sizeof(temp_file), "%s\\xcd_target.txt", tmp);
    temp_file[sizeof(temp_file) - 1] = '\0';

    load_memory();

    /* --- Management / info commands (no starting-dir remembering) --- */

    if (argc >= 2)
    {
        const char *arg1 = argv[1];

        if (strcmp(arg1, "-h") == 0)
        {
            print_help();
            return 0;
        }

        if (strcmp(arg1, "-c") == 0)
        {
            /* Clear in-memory list and truncate file */
            for (int i = 0; i < memcount; i++)
            {
                free(memory[i]);
                memory[i] = NULL;
            }
            memcount = 0;
            save_memory();
            return 0;
        }

        if (strcmp(arg1, "-l") == 0)
        {
            if (argc == 2)
            {
                for (int i = 0; i < memcount; i++)
                    printf("%s\n", memory[i]);
                return 0;
            }
            else
            {
                const char *seg = argv[2];
                for (int i = 0; i < memcount; i++)
                {
                    const char *base = strrchr(memory[i], '\\');
                    if (!base)
                        base = memory[i];
                    else
                        base++;

                    if (strstr(base, seg) != NULL)
                        printf("%s\n", memory[i]);
                }
                return 0;
            }
        }

        if (strcmp(arg1, "-p") == 0)
        {
            if (argc < 3)
            {
                fprintf(stderr, "Usage: xcd -p segment\n");
                return 1;
            }

            const char *seg = argv[2];
            int idxs[MAXDIRS];
            int count = find_matches(seg, idxs, MAXDIRS);

            if (count == 0)
            {
                printf("No matches for \"%s\".\n", seg);
                return 0;
            }

            char cwd[MAXPATH];
            if (!_getcwd(cwd, sizeof(cwd)))
            {
                fprintf(stderr, "xcd: cannot determine current directory\n");
                return 1;
            }

            int cur = -1;
            for (int i = 0; i < count; i++)
            {
                if (_stricmp(memory[idxs[i]], cwd) == 0)
                {
                    cur = i;
                    break;
                }
            }

            int next = (cur < 0 ? 0 : (cur + 1) % count);

            printf("Matches for \"%s\":\n", seg);
            for (int i = 0; i < count; i++)
            {
                const char *mark = (i == cur ? "*" : " ");
                printf("  [%d]%s %s\n", i, mark, memory[idxs[i]]);
            }

            if (cur >= 0)
                printf("Current dir is at index [%d].\n", cur);
            else
                printf("Current dir is not one of the matches.\n");

            printf("Next target on xcd \"%s\": [%d] %s\n",
                   seg, next, memory[idxs[next]]);

            return 0;
        }
    }

    /* --- Navigation mode: remember starting directory, then pick target --- */

    /* 1. Remember the directory we are currently in */
    char cwd[MAXPATH];
    if (_getcwd(cwd, sizeof(cwd)))
    {
        remember_dir(cwd);
    }

    /* 2. Decide where to go (target) and write it to temp file */

    if (argc == 1)
    {
        /* No args: go to home directory */
        char full[MAXPATH];
        if (_fullpath(full, home, MAXPATH))
        {
            write_target(full);
            remember_dir(full);
            return 0;
        }
        else
        {
            fprintf(stderr, "xcd: cannot resolve home directory\n");
            return 1;
        }
    }

    const char *arg = argv[1];

    /* Direct path exists? Go there */
    if (is_dir(arg))
    {
        char full[MAXPATH];
        if (!_fullpath(full, arg, MAXPATH))
        {
            fprintf(stderr, "xcd: _fullpath failed for %s\n", arg);
            return 1;
        }

        write_target(full);
        remember_dir(full);
        return 0;
    }

    /* Fuzzy match on basename if no slash and not an existing dir */
    if (strchr(arg, '\\') == NULL && strchr(arg, '/') == NULL)
    {
        const char *seg = arg;
        int idxs[MAXDIRS];
        int count = find_matches(seg, idxs, MAXDIRS);

        if (count == 0)
        {
            fprintf(stderr, "xcd: no directory matches \"%s\"\n", seg);
            return 1;
        }

        if (!_getcwd(cwd, sizeof(cwd)))
        {
            fprintf(stderr, "xcd: cannot determine current directory\n");
            return 1;
        }

        int cur = -1;
        for (int i = 0; i < count; i++)
        {
            if (_stricmp(memory[idxs[i]], cwd) == 0)
            {
                cur = i;
                break;
            }
        }

        int next = (cur < 0 ? 0 : (cur + 1) % count);
        const char *target = memory[idxs[next]];

        write_target(target);
        remember_dir(target);
        return 0;
    }

    /* Otherwise, invalid segment/path */
    fprintf(stderr, "xcd: \"%s\" is not a directory and not a simple segment\n", arg);
    return 1;
}


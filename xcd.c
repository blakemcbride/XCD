#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <direct.h>
#include <stdlib.h>

#define MAXDIRS 4096
#define MAXPATH 4096

static char *memory[MAXDIRS];
static int memcount = 0;

static char memory_file[MAXPATH];
static char temp_file[MAXPATH];

/* Read memory file into RAM */
void load_memory()
{
    FILE *f = fopen(memory_file, "r");
    if(!f) return;

    char buf[MAXPATH];

    while(memcount < MAXDIRS && fgets(buf, sizeof(buf), f))
    {
        char *nl = strchr(buf, '\n');
        if(nl) *nl = '\0';

        if(strlen(buf) == 0)
            continue;

        if(GetFileAttributesA(buf) == INVALID_FILE_ATTRIBUTES)
            continue; /* skip dead entries */

        memory[memcount] = _strdup(buf);
        memcount++;
    }

    fclose(f);
}

/* Save memory file */
void save_memory()
{
    FILE *f = fopen(memory_file, "w");
    if(!f) return;

    for(int i=0; i<memcount; i++)
        fprintf(f, "%s\n", memory[i]);

    fclose(f);
}

/* Add a directory to memory if not present */
void remember_dir(const char *dir)
{
    for(int i=0; i<memcount; i++)
        if(strcmp(memory[i], dir) == 0)
            return;

    if(memcount < MAXDIRS)
    {
        memory[memcount++] = _strdup(dir);
    }

    save_memory();
}

/* Write directory to temp file */
void write_target(const char *dir)
{
    FILE *f = fopen(temp_file, "w");
    if(!f) return;

    fprintf(f, "%s", dir);
    fclose(f);
}

/* Print help */
void print_help()
{
    printf("xcd - Windows directory jumper\n\n");
    printf("Usage:\n");
    printf("  xcd <segment>      fuzzy match directory\n");
    printf("  xcd -l             list all remembered directories\n");
    printf("  xcd -l segment     list matches for segment\n");
    printf("  xcd -c             clear memory\n");
    printf("  xcd -p segment     preview cycle order\n");
    printf("  xcd -h             help\n");
}

/* main match logic */
int find_matches(const char *seg, int *out_indices, int max)
{
    int count = 0;

    for(int i=0; i<memcount; i++)
    {
        const char *base = strrchr(memory[i], '\\');
        if(!base) base = memory[i];
        else base++;

        if(strstr(base, seg) != NULL) /* fuzzy match */
        {
            if(count < max)
                out_indices[count] = i;
            count++;
        }
    }

    return count;
}

int main(int argc, char **argv)
{
    /* Build paths */
    const char *home = getenv("USERPROFILE");
    snprintf(memory_file, sizeof(memory_file), "%s\\.xcd_memory", home);

    char tmp[MAXPATH];
    GetTempPathA(sizeof(tmp), tmp);
    snprintf(temp_file, sizeof(temp_file), "%s\\xcd_target.txt", tmp);

    load_memory();

    /* No args: return HOME */
    if(argc == 1)
    {
        write_target(home);
        remember_dir(home);
        return 0;
    }

    /* -h */
    if(strcmp(argv[1], "-h") == 0)
    {
        print_help();
        return 0;
    }

    /* -c */
    if(strcmp(argv[1], "-c") == 0)
    {
        memcount = 0;
        save_memory();
        return 0;
    }

    /* -l [segment] */
    if(strcmp(argv[1], "-l") == 0)
    {
        if(argc == 2)
        {
            for(int i=0; i<memcount; i++)
                printf("%s\n", memory[i]);

            return 0;
        }
        else
        {
            const char *seg = argv[2];
            for(int i=0; i<memcount; i++)
            {
                const char *base = strrchr(memory[i], '\\');
                if(!base) base = memory[i];
                else base++;

                if(strstr(base, seg))
                    printf("%s\n", memory[i]);
            }
            return 0;
        }
    }

    /* -p segment */
    if(strcmp(argv[1], "-p") == 0)
    {
        if(argc < 3)
        {
            fprintf(stderr, "Missing segment.\n");
            return 1;
        }

        const char *seg = argv[2];
        int idxs[MAXDIRS];
        int count = find_matches(seg, idxs, MAXDIRS);

        if(count == 0)
        {
            printf("No matches for \"%s\".\n", seg);
            return 0;
        }

        printf("Matches for \"%s\":\n", seg);
        for(int i=0; i<count; i++)
            printf("  [%d] %s\n", i, memory[idxs[i]]);

        return 0;
    }

    /* Normal fuzzy navigation */
    const char *seg = argv[1];

    /* If it's a real directory, go there directly */
    if(GetFileAttributesA(seg) != INVALID_FILE_ATTRIBUTES)
    {
        char full[MAXPATH];
        _fullpath(full, seg, MAXPATH);
        write_target(full);
        remember_dir(full);
        return 0;
    }

    /* fuzzy */
    int idxs[MAXDIRS];
    int count = find_matches(seg, idxs, MAXDIRS);

    if(count == 0)
    {
        fprintf(stderr, "No directory matches \"%s\".\n", seg);
        return 1;
    }

    /* Determine current physical directory */
    char cwd[MAXPATH];
    _getcwd(cwd, sizeof(cwd));

    int cur = -1;
    for(int i=0; i<count; i++)
    {
        if(strcmp(memory[idxs[i]], cwd) == 0)
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


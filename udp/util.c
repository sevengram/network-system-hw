#include <string.h>
#include <dirent.h>
#include <stdio.h>

int is_endl(char *line)
{
    return strlen(line) == 1 && line[0] == '\n';
}

void remove_endl(char *line)
{
    size_t n = strlen(line);
    if (n > 0 && line[n - 1] == '\n') {
        line[n - 1] = '\0';
    }
}

int parse_line(char *cmdline, char **argv)
{
    char *p;
    int i = 0;
    remove_endl(cmdline);
    if (strlen(cmdline) == 0) {
        return 0;
    }
    while ((p = strsep(&cmdline, " ")) != NULL) {
        argv[i++] = p;
    }
    return i;
}

void list_dir(char *buffer, const char *name, char delimiter)
{
    struct dirent *dir;
    DIR *d = opendir(name);
    size_t pos = 0;
    buffer[0] = '\0';
    if (d) {
        while ((dir = readdir(d)) != NULL) {
            if (strcmp(dir->d_name, ".") && strcmp(dir->d_name, "..")) {
                sprintf(buffer + pos, "%s%c", dir->d_name, delimiter);
                pos = strlen(buffer);
            }
        }
        closedir(d);
    }
}
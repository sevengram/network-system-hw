#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include "util.h"

void format_current_time(char *buf, size_t maxsize)
{
    time_t now = time(0);
    struct tm gt = *gmtime(&now);
    strftime(buf, maxsize, "%a, %d %b %Y %H:%M:%S %Z", &gt);
}

long size_of_file(const char *filename)
{
    struct stat st;
    if (stat(filename, &st) == -1) {
        return -1;
    } else {
        return st.st_size;
    }
}

char **malloc_strings(size_t n, size_t len)
{
    int i;
    char **c = calloc(n, sizeof(char *));
    for (i = 0; i < n; i++) {
        c[i] = calloc(len, sizeof(char));
    }
    return c;
}

void free_strings(char **c, size_t n)
{
    int i;
    for (i = 0; i < n; i++) {
        free(c[i]);
    }
    free(c);
}

void get_file_extension(const char *filename, char *extension)
{
    char *p = strrchr(filename, '.');
    if (p != 0) {
        strcpy(extension, p);
    } else {
        strcpy(extension, "");
    }
}
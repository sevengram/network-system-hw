#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "strutil.h"

/*
 * Remove \n or \r or \r\n
 */
void remove_endl(char *line)
{
    size_t n = strlen(line);
    if (n > 0 && line[n - 1] == '\n') {
        line[n - 1] = '\0';
    }
    n = strlen(line);
    if (n > 0 && line[n - 1] == '\r') {
        line[n - 1] = '\0';
    }
}

/*
 * Remove leading and ending blanks
 */
void strip(char *line, char sep)
{
    char *p = line;
    while (*p == sep) {
        p++;
    }
    strcpy(line, p);
    size_t n = strlen(line);
    if (n > 1) {
        p = line + n - 1;
    }
    while (*p == sep) {
        *p = '\0';
        p--;
    }
}

/*
 * Split str
 */
int split(char *str, char *sep, char **results)
{
    size_t n = strlen(str);
    if (n == 0) {
        return 0;
    }
    char *p;
    int i = 0;
    char *buf = calloc(n + 1, sizeof(char));
    strcpy(buf, str);
    while ((p = strsep(&buf, sep)) != 0) {
        if (strlen(p) != 0) {
            strcpy(results[i], p);
            i++;
        }
    }
    free(buf);
    return i;
}

void split_first(char *str, char sep, char *first, char *second)
{
    char *p = strchr(str, sep);
    if (p != 0) {
        *p = '\0';
        strcpy(first, str);
        strcpy(second, p + 1);
    } else {
        strcpy(first, str);
        strcpy(second, "");
    }
}

/*
 * To lowercase
 */
void lower(char *str)
{
    int i;
    for (i = 0; str[i]; i++) {
        str[i] = tolower(str[i]);
    }
}
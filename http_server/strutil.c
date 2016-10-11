//
// Created by jfan on 10/11/16.
//

#include <string.h>
#include <stdlib.h>
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
void strip(char *line)
{
    char *p = line;
    while (*p == ' ') {
        p++;
    }
    strcpy(line, p);
    size_t n = strlen(line);
    if (n > 1) {
        p = line + n - 1;
    }
    while (*p == ' ') {
        *p = '\0';
        p--;
    }
}

/*
 * Split str
 */
int split(char *str, char *sep, char results[][LINE_LEN_MAX])
{
    size_t n = strlen(str);
    if (n == 0) {
        return 0;
    }
    char *buf = malloc(n);
    char *p;
    int i = 0;
    strcpy(buf, str);
    while ((p = strsep(&buf, sep)) != NULL) {
        strcpy(results[i], p);
        i++;
    }
    free(buf);
    return i;
}
#ifndef HTTP_SERVER_UTIL_H
#define HTTP_SERVER_UTIL_H

#include "strutil.h"

void format_current_time(char *buf, size_t maxsize);

long size_of_file(const char *filename);

char **malloc_strings(size_t n, size_t len);

void free_strings(char **c, size_t n);

void get_file_extension(const char *filename, char *extension);

#endif //HTTP_SERVER_UTIL_H

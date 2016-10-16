#ifndef HTTP_SERVER_UTIL_H
#define HTTP_SERVER_UTIL_H

#include "strutil.h"

ssize_t rio_writen(int fd, void *usrbuf, size_t n);

void format_current_time(char *buf, size_t maxsize);

#endif //HTTP_SERVER_UTIL_H

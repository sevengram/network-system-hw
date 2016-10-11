#ifndef HTTP_SERVER_UTIL_H
#define HTTP_SERVER_UTIL_H

#include "strutil.h"

#define LINE_MAX 32

ssize_t rio_writen(int fd, void *usrbuf, size_t n);

void build_http_resp(char *buf, int status, char *content, size_t length, int keep_alive, int content_type);

void parse_http_request(char *buf, char lines[][LINE_LEN_MAX]);

#endif //HTTP_SERVER_UTIL_H

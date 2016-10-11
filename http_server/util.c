//
// Created by jfan on 10/11/16.
//

#include <stdio.h>
#include <time.h>
#include <errno.h>
#include <unistd.h>
#include "util.h"

const char *http_resp_format = "HTTP/1.1 %d %s\r\nDate: %s\r\nContent-Length: %d\r\nConnection: %s\r\nContent-Type: %s\r\n\r\n%s";

void format_current_time(char *buf, size_t maxsize)
{
    time_t now = time(0);
    struct tm gt = *gmtime(&now);
    strftime(buf, maxsize, "%a, %d %b %Y %H:%M:%S %Z", &gt);
}

/*
 * rio_writen - robustly write n bytes (unbuffered)
 */
ssize_t rio_writen(int fd, void *usrbuf, size_t n)
{
    size_t nleft = n;
    ssize_t nwritten;
    char *bufp = usrbuf;

    while (nleft > 0) {
        if ((nwritten = write(fd, bufp, nleft)) <= 0) {
            if (errno == EINTR)  /* interrupted by sig handler return */
                nwritten = 0;    /* and call write() again */
            else
                return -1;       /* errno set by write() */
        }
        nleft -= nwritten;
        bufp += nwritten;
    }
    return n;
}

void parse_http_request(char *buf, char lines[][LINE_LEN_MAX])
{
    int n = split(buf, "\n", lines);
}

void build_http_resp(char *buf, int status, char *content, size_t length, int keep_alive, int content_type)
{
    char time_buf[128];
    format_current_time(time_buf, sizeof(time_buf));
    printf(http_resp_format, status, "OK", time_buf, length, "keep-alive", "text/html", content);
    sprintf(buf, http_resp_format, status, "OK", time_buf, length, "keep-alive", "text/html", content);
}
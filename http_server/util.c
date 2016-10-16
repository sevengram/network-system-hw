#include <stdio.h>
#include <time.h>
#include <errno.h>
#include <unistd.h>
#include "util.h"

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

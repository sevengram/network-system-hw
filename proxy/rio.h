//
// Created by jfan on 12/6/16.
//

#ifndef NETWORKS_RIO_H
#define NETWORKS_RIO_H

#define RIO_BUFSIZE 65536
typedef struct
{
    int rio_fd;
    /* descriptor for this internal buf */
    int rio_cnt;
    /* unread bytes in internal buf */
    char *rio_bufptr;
    /* next unread byte in internal buf */
    char rio_buf[RIO_BUFSIZE]; /* internal buffer */
} rio_t;

/* Wrappers for Rio package */
ssize_t Rio_readn(int fd, void *usrbuf, size_t n);

void Rio_writen(int fd, void *usrbuf, size_t n);

void Rio_readinitb(rio_t *rp, int fd);

ssize_t Rio_readnb(rio_t *rp, void *usrbuf, size_t n);

ssize_t Rio_readlineb(rio_t *rp, void *usrbuf, size_t maxlen);

#endif //NETWORKS_RIO_H

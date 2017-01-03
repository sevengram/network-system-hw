#include <time.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <netdb.h>
#include "pthread.h"
#include "log.h"

#define DEFAULT_PORT 80

#define MAXBUF  4096
#define LISTENQ 1024

int parse_address(char *uri, char *address, char *path, int *port);

void send_error_msg(int fd, char *errnum, char *shortmsg, char *longmsg);

int read_request(int fd, char *address, int *port, char *path, char *uri, char *version);

void send_request(int fd, char *address, char *path, char *version);

void *connection_handler(void *vargp);

int open_listenfd(int port)
{
    int listenfd, optval = 1;
    struct sockaddr_in serveraddr;

    /* Create a socket descriptor */
    if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        return -1;

    /* Eliminates "Address already in use" error from bind. */
    if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR,
                   (const void *) &optval, sizeof(int)) < 0)
        return -1;

    /* Listenfd will be an endpoint for all requests to port
       on any IP address for this host */
    bzero((char *) &serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serveraddr.sin_port = htons((unsigned short) port);
    if (bind(listenfd, (struct sockaddr *) &serveraddr, sizeof(serveraddr)) < 0)
        return -1;

    /* Make it a listening socket ready to accept connection requests */
    if (listen(listenfd, LISTENQ) < 0)
        return -1;
    return listenfd;
}

int main(int argc, char *argv[])
{
    int listenfd, *connfd;
    pthread_t tid;
    struct sockaddr_in clientaddr;
    socklen_t clientlen = sizeof(clientaddr);

    if (argc < 2) {
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        exit(1);
    }
    listenfd = open_listenfd(atoi(argv[1]));
    while (1) {
        connfd = malloc(1);
        *connfd = accept(listenfd, (struct sockaddr *) &clientaddr, &clientlen);
        if (pthread_create(&tid, NULL, connection_handler, connfd) < 0) {
            perror("could not create thread");
            return 1;
        }
    }
    close(listenfd);
    return 0;
}

int open_clientfd(char *hostname, int port)
{
    int clientfd;
    struct hostent *hp;
    struct sockaddr_in serveraddr;

    if ((clientfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        return -1;

    /* Fill in the server's IP address and port */
    if ((hp = gethostbyname(hostname)) == NULL)
        return -2;

    bzero((char *) &serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    bcopy(hp->h_addr_list[0], (char *) &serveraddr.sin_addr.s_addr, hp->h_length);
    serveraddr.sin_port = htons(port);

    /* Establish a connection with the server */
    if (connect(clientfd, (struct sockaddr *) &serveraddr, sizeof(serveraddr)) < 0)
        return -1;
    return clientfd;
}

void *connection_handler(void *vargp)
{
    pthread_detach(pthread_self());
    int connfd = *(int *) vargp;
    int proxyfd;
    char buf[MAXBUF];
    char uri[MAXBUF], version[MAXBUF];
    char address[MAXBUF];
    int port;
    char path[MAXBUF];
    ssize_t n = 0;
    int total_size = 0;

    if (read_request(connfd, address, &port, path, uri, version) == -1) {
        return 0;
    }

    proxyfd = open_clientfd(address, port);
    send_request(proxyfd, address, path, version);

    while ((n = recv(proxyfd, buf, MAXBUF, 0)) > 0) {
        total_size += n;
        send(connfd, buf, n, 0);
    }

    log_info(connfd, uri, total_size);

    close(proxyfd);
    close(connfd);
    return 0;
}

int read_request(int fd, char *address, int *port, char *path,
                 char *uri, char *version)
{
    char method[MAXBUF];
    char buf[MAXBUF];

    ssize_t n = recv(fd, buf, MAXBUF, 0);
    if (n == 0) {
        return -1;
    }
    sscanf(buf, "%s %s %s", method, uri, version);       //parserequest

    if (strcasecmp(method, "GET")) {
        printf("Unsupported method:%s\n", method);
        return -1;
    }
    if (!parse_address(uri, address, path, port)) {
        send_error_msg(fd, "400", "Bad request", "Proxy cannot parse the address");
        return -1;
    }
    return 0;
}

void send_request(int fd, char *address, char *path, char *version)
{
    char buf[MAXBUF];
    sprintf(buf, "GET /%s %s\r\nHost: %s\r\n\r\n", path, version, address);
    send(fd, buf, strlen(buf), 0);
}

int parse_address(char *uri, char *address, char *path, int *port)
{
    if (strncasecmp(uri, "http://", 7) == 0) {
        char *hostbegin, *hostend, *pathbegin;
        int len;

        /* find the target address */
        hostbegin = uri + 7;
        hostend = strpbrk(hostbegin, " :/\r\n");
        if (hostend == NULL) {
            hostend = hostbegin + strlen(hostbegin);
        }

        len = hostend - hostbegin;
        strncpy(address, hostbegin, len);
        address[len] = '\0';
        if (*hostend == ':')
            *port = atoi(hostend + 1);
        else
            *port = DEFAULT_PORT;
        pathbegin = strchr(hostbegin, '/');

        if (pathbegin == NULL) {
            path[0] = '\0';
        } else {
            pathbegin++;
            strcpy(path, pathbegin);
        }
        return 1;
    }
    address[0] = '\0';
    return 0;
}

void send_error_msg(int fd, char *errnum, char *shortmsg, char *longmsg)
{
    char buf[MAXBUF];
    sprintf(buf, "HTTP/1.0 %s %s\r\nContent-type: text/html\r\nContent-length: %d\r\n\r\n%s", errnum, shortmsg,
            (int) strlen(longmsg), longmsg);
    send(fd, buf, strlen(buf), 0);
}

/*
 * proxy.c - Web proxy for Lab 8
 */

#include <time.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <netdb.h>
#include "pthread.h"
#include "rio.h"
#include "log.h"

#define DEFAULT_PORT 80

#define ERROR -1
#define SUCCESS 0
#define MAXLINE 4096
#define MAXBUF  4096
#define LISTENQ 1024

int find_target_address(char *uri, char *target_address, char *path, int *port);

void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg);

void read_requesthdrs(rio_t *rp);

int read_request(int fd, char *address, int *port, char *path, char *uri, char *version);

void send_request(int fd, char *address, char *path, char *version);

void *connection_handler(void *vargp);

int open_listenfd(int port);

int main(int argc, char *argv[])
{
    int listenfd, *connfd;
    pthread_t tid;
    struct sockaddr_in clientaddr;
    socklen_t clientlen = sizeof(clientaddr);

    if (argc != 2) {
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

int open_clientfd(char *hostname, int port)
{
    int clientfd;
    struct hostent *hp;
    struct sockaddr_in serveraddr;

    if ((clientfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        return -1; /* check errno for cause of error */

    /* Fill in the server's IP address and port */
    if ((hp = gethostbyname(hostname)) == NULL)
        return -2; /* check h_errno for cause of error */

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
    rio_t rio;

    /* request url from the client */
    char uri[MAXLINE], version[MAXLINE];

    /* address and port of the web server */
    char address[MAXLINE];
    int port;

    /* path of the content */
    char path[MAXLINE];

    /* count variables */
    int n = 0;
    int total_size = 0;

    /* get request from client */
    if (read_request(connfd, address, &port, path, uri, version) == ERROR)
        return 0;

    /* open connection to the destination server */
    proxyfd = open_clientfd(address, port);
    Rio_readinitb(&rio, proxyfd);

    /* send request to server */
    send_request(proxyfd, address, path, version);

    /* response request */
    while ((n = Rio_readlineb(&rio, buf, MAXBUF)) > 0) {
        total_size += n;
        Rio_writen(connfd, buf, n);
    }

    log_info(connfd, uri, total_size);

    /* Clean up then return */
    close(proxyfd);
    close(connfd);
    return 0;
}

int read_request(int fd, char *address, int *port, char *path,
                 char *uri, char *version)
{
    char method[MAXLINE];
    char buf[MAXLINE];
    rio_t rio;

    Rio_readinitb(&rio, fd);
    Rio_readlineb(&rio, buf, MAXLINE);                   //readrequest
    sscanf(buf, "%s %s %s", method, uri, version);       //parserequest

    if (strcasecmp(method, "GET")) {
        clienterror(fd, method, "501", "Not Implemented",
                    "Proxy does not implement this method");
        return ERROR;
    }

    read_requesthdrs(&rio);

    /* Parse URI from GET request */
    if (!find_target_address(uri, address, path, port)) {
        clienterror(fd, uri, "400", "Bad request",
                    "Proxy cannot parse the address");
        return ERROR;
    }
    return SUCCESS;
}

void send_request(int fd, char *address,
                  char *path, char *version)
{
    char buf[MAXLINE];

    /* send requests */
    sprintf(buf, "GET /%s %s\r\n", path, version);
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Host: %s\r\n\r\n", address);
    Rio_writen(fd, buf, strlen(buf));
}


int find_target_address(char *uri, char *target_address, char *path, int *port)
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

        strncpy(target_address, hostbegin, len);
        target_address[len] = '\0';

        /* find the port number */
        if (*hostend == ':')
            *port = atoi(hostend + 1);
        else
            *port = DEFAULT_PORT;

        /* find the path */

        pathbegin = strchr(hostbegin, '/');

        if (pathbegin == NULL) {
            path[0] = '\0';
        }
        else {
            pathbegin++;
            strcpy(path, pathbegin);
        }
        return 1;
    }

    target_address[0] = '\0';

    return 0;
}

void clienterror(int fd, char *cause, char *errnum,
                 char *shortmsg, char *longmsg)
{
    char buf[MAXLINE], body[MAXBUF];

    /* Build the HTTP response body */
    sprintf(body, "<html><title>Proxy Error</title>");
    sprintf(body, "%s<body bgcolor=""ffffff"">\r\n", body);
    sprintf(body, "%s%s: %s\r\n", body, errnum, shortmsg);
    sprintf(body, "%s<p>%s: %s\r\n", body, longmsg, cause);
    sprintf(body, "%s<hr><em>The FProxy server</em>\r\n", body);

    /* Print the HTTP response */
    sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg);
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Content-type: text/html\r\n");
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Content-length: %d\r\n\r\n", (int) strlen(body));
    Rio_writen(fd, buf, strlen(buf));
    Rio_writen(fd, body, strlen(body));
}

void read_requesthdrs(rio_t *rp)
{
    char buf[MAXLINE];

    Rio_readlineb(rp, buf, MAXLINE);
    while (strcmp(buf, "\r\n"))
        Rio_readlineb(rp, buf, MAXLINE);

    return;
}

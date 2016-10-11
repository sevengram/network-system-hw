#include <netinet/in.h>
#include <sys/epoll.h>
#include <sys/resource.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "util.h"


#define EPOLL_SIZE_MAX  1000
#define LISTENQ         1024  /* second argument to listen() */
#define REQ_BUF_SIZE    1024
#define RESP_BUF_SIZE   1024*32

void perror_exit(const char *msg);

int setnonblocking(int sockfd);

int handle(int connfd);

int main(int argc, char *argv[])
{
    int listenfd, epollfd, connfd;
    struct sockaddr_in server, client;
    struct rlimit rt;

    // set limitation for the number of file descriptor opened
    rt.rlim_max = rt.rlim_cur = EPOLL_SIZE_MAX;
    if (setrlimit(RLIMIT_NOFILE, &rt) == -1) {
        perror_exit("setrlimit error");
    }

    // Create socket
    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if (listenfd == -1) {
        perror_exit("Could not create socket");
    }

    // Eliminates "Address already in use" error from bind.
    int optval = 1;
    if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, (const void *) &optval, sizeof(int)) < 0) {
        perror_exit("unable to call setsockopt");
    }

    if (setnonblocking(listenfd) < 0) {
        perror_exit("setnonblock error");
    }

    //Prepare the sockaddr_in structure
    bzero(&server, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(8888); // TODO: hard code port

    if (bind(listenfd, (struct sockaddr *) &server, sizeof(server)) < 0) {
        perror_exit("bind failed. Error");
    }
    if (listen(listenfd, LISTENQ) < 0) {
        perror_exit("listen error");
    }

    struct epoll_event ev;
    struct epoll_event events[EPOLL_SIZE_MAX];
    epollfd = epoll_create(EPOLL_SIZE_MAX);
    ev.events = EPOLLIN | EPOLLET;
    ev.data.fd = listenfd;
    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, listenfd, &ev) < 0) {
        fprintf(stderr, "epoll set insertion error: fd=%d\n", listenfd);
        return -1;
    }

    int n;
    int nfds;
    int max_events = 1;
    socklen_t socklen = sizeof(struct sockaddr_in);
    while (1) {
        nfds = epoll_wait(epollfd, events, max_events, -1);
        if (nfds == -1) {
            perror("epoll_wait");
            continue;
        }
        for (n = 0; n < nfds; ++n) {
            if (events[n].data.fd == listenfd) {
                connfd = accept(listenfd, (struct sockaddr *) &client, &socklen);
                if (connfd < 0) {
                    perror("accept error");
                    continue;
                }
                if (max_events >= EPOLL_SIZE_MAX) {
                    perror("too many connection");
                    close(connfd);
                    continue;
                }
                if (setnonblocking(connfd) < 0) {
                    perror("setnonblocking error");
                }
                ev.events = EPOLLIN | EPOLLET;
                ev.data.fd = connfd;
                if (epoll_ctl(epollfd, EPOLL_CTL_ADD, connfd, &ev) < 0) {
                    perror("fail to add socket to epoll");
                    close(connfd);
                    continue;
                }
                max_events++;
            } else {
                if (handle(events[n].data.fd) < 0) {
                    epoll_ctl(epollfd, EPOLL_CTL_DEL, events[n].data.fd, &ev);
                    max_events--;
                }
            }
        }
    }
    close(listenfd);
    return 0;
}

char *sample_resp = "<html>\n<body>\n<h1>test</h1>\n</body>\n</html>\n";

int handle(int connfd)
{
    char req_buf[REQ_BUF_SIZE];
    char resp_buf[RESP_BUF_SIZE];
    bzero(req_buf, REQ_BUF_SIZE);
    char lines[LINE_MAX][LINE_LEN_MAX];

    // Receive a message from client
    while (recv(connfd, req_buf, REQ_BUF_SIZE, 0) > 0) {
        bzero(resp_buf, RESP_BUF_SIZE);
        parse_http_request(req_buf, lines);

        // Send the message back to client
        build_http_resp(resp_buf, 200, sample_resp, strlen(sample_resp), 1, 1);
        rio_writen(connfd, resp_buf, strlen(resp_buf));

        bzero(req_buf, REQ_BUF_SIZE);
    }
    return 0;
}

void perror_exit(const char *msg)
{
    perror(msg);
    exit(1);
}

int setnonblocking(int sockfd)
{
    if (fcntl(sockfd, F_SETFL, fcntl(sockfd, F_GETFD, 0) | O_NONBLOCK) == -1) {
        return -1;
    }
    return 0;
}
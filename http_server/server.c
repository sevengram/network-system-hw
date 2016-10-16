#include <netinet/in.h>
#include <sys/epoll.h>
#include <sys/resource.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <errno.h>
#include "http.h"
#include "util.h"

#define EPOLL_SIZE_MAX  1000
#define LISTENQ         1024  /* second argument to listen() */
#define BUF_SIZE        4096

void perror_exit(const char *msg);

int setnonblocking(int sockfd);

int handle_in(int epollfd, struct epoll_event ev);

int handle_out(int epollfd, struct epoll_event ev);

int handle_accept(int epollfd, struct epoll_event ev, int max_events);

typedef struct
{
    int fd;
    void *data;
} epoll_io_t;

int main(int argc, char *argv[])
{
    int listenfd, epollfd;
    struct sockaddr_in server;
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
        perror_exit("bind failed");
    }
    if (listen(listenfd, LISTENQ) < 0) {
        perror_exit("listen error");
    }

    struct epoll_event ev;
    struct epoll_event events[EPOLL_SIZE_MAX];
    epollfd = epoll_create(EPOLL_SIZE_MAX);
    ev.events = EPOLLIN | EPOLLET;
    ev.data.fd = listenfd;
    epoll_ctl(epollfd, EPOLL_CTL_ADD, listenfd, &ev);

    int i, n;
    int events_count = 1;
    while (1) {
        n = epoll_wait(epollfd, events, events_count, -1);
        if (n == -1) {
            perror("epoll_wait");
            continue;
        }
        for (i = 0; i < n; ++i) {
            if (events[i].data.fd == listenfd) {
                handle_accept(epollfd, events[i], events_count++);
            } else if (events[i].events & EPOLLIN) {
                handle_in(epollfd, events[i]);
            } else if (events[i].events & EPOLLOUT) {
                handle_out(epollfd, events[i]);
            }
        }
    }
    close(listenfd);
    return 0;
}

char *sample_resp = "<html>\n<body>\n<h1>test</h1>\n</body>\n</html>\n";

int handle_accept(int epollfd, struct epoll_event ev, int max_events)
{
    struct sockaddr_in client;
    int listenfd = ev.data.fd;
    epoll_io_t *io_data = calloc(1, sizeof(epoll_io_t));
    socklen_t socklen = sizeof(struct sockaddr_in);
    int connfd = accept(listenfd, (struct sockaddr *) &client, &socklen);
    if (connfd == -1) {
        if ((errno == EAGAIN) || (errno == EWOULDBLOCK)) {
            return 0;
        } else {
            perror("accept error!");
            return -1;
        }
    }
    if (max_events >= EPOLL_SIZE_MAX) {
        perror("too many connection");
        close(connfd);
        return -1;
    }
    if (setnonblocking(connfd) < 0) {
        perror("setnonblocking error");
    }
    io_data->fd = connfd;
    ev.data.ptr = io_data;
    ev.events = EPOLLIN | EPOLLET;
    epoll_ctl(epollfd, EPOLL_CTL_ADD, connfd, &ev);
    return 0;
}

int handle_in(int epollfd, struct epoll_event ev)
{
    char buf[BUF_SIZE];
    epoll_io_t *io_data = ev.data.ptr;
    http_req_t *req = calloc(1, sizeof(http_req_t));
    int connfd = io_data->fd;

    // Receive a message from client
    bzero(buf, BUF_SIZE);
    recv(connfd, buf, BUF_SIZE, 0);
    *req = parse_http_req(buf);

    io_data->fd = connfd;
    io_data->data = req;
    ev.events = EPOLLOUT | EPOLLET;
    epoll_ctl(epollfd, EPOLL_CTL_MOD, connfd, &ev);
    return 0;
}

int handle_out(int epollfd, struct epoll_event ev)
{
    char buf[BUF_SIZE];
    epoll_io_t *io_data = ev.data.ptr;
    http_req_t *req = io_data->data;
    int connfd = io_data->fd;

    http_resp_header_t resp_header;
    resp_header.version = req->version;
    resp_header.keep_alive = req->keep_alive;
    resp_header.content_type = 1;
    resp_header.status_code = 200;
    resp_header.content_length = strlen(sample_resp);

    build_http_resp_header(buf, &resp_header);
    rio_writen(connfd, buf, strlen(buf));
    strcpy(buf, sample_resp);
    rio_writen(connfd, buf, strlen(buf));

    io_data->fd = connfd;
    free(io_data->data);
    io_data->data = 0;
    ev.events = EPOLLIN | EPOLLET;
    epoll_ctl(epollfd, EPOLL_CTL_MOD, connfd, &ev);
    return 0;
}

void perror_exit(const char *msg)
{
    perror(msg);
    exit(1);
}

int setnonblocking(int sockfd)
{
    if (fcntl(sockfd, F_SETFL, fcntl(sockfd, F_GETFL, 0) | O_NONBLOCK) == -1) {
        return -1;
    }
    return 0;
}
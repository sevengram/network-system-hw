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
#include <time.h>
#include "http.h"
#include "util.h"

#define EPOLL_SIZE_MAX  1024
#define LISTENQ         1024

int setnonblocking(int sockfd);

int is_timeout(struct epoll_event *ev);

int handle_in(int epollfd, struct epoll_event *ev, config_t *conf);

int handle_out(int epollfd, struct epoll_event *ev, config_t *conf);

int handle_accept(int epollfd, struct epoll_event *ev);

void unregister_event(int epollfd, struct epoll_event *ev);

int events_count = 1;

int main(int argc, char *argv[])
{
    int listenfd, epollfd;
    struct sockaddr_in server;
    struct rlimit rt;

    config_t conf;
    if (init_config(&conf, argc > 1 ? argv[1] : "ws.conf") == -1) {
        printf("Invalid config file.\n");
        exit(1);
    }

    if (conf.port < 1024) {
        printf("Port number should be >= 1024.\n");
        exit(1);
    }

    // set limitation for the number of file descriptor opened
    rt.rlim_max = rt.rlim_cur = EPOLL_SIZE_MAX;
    if (setrlimit(RLIMIT_NOFILE, &rt) == -1) {
        perror("setrlimit error");
        exit(1);
    }

    // Create socket
    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if (listenfd == -1) {
        perror("Could not create socket");
        exit(1);
    }

    // Eliminates "Address already in use" error from bind.
    int optval = 1;
    if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, (const void *) &optval, sizeof(int)) < 0) {
        perror("unable to call setsockopt");
        exit(1);
    }

    if (setnonblocking(listenfd) < 0) {
        perror("setnonblock error");
        exit(1);
    }

    //Prepare the sockaddr_in structure
    bzero(&server, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(conf.port);

    if (bind(listenfd, (struct sockaddr *) &server, sizeof(server)) < 0) {
        perror("bind failed");
        exit(1);
    }
    if (listen(listenfd, LISTENQ) < 0) {
        perror("listen error");
        exit(1);
    }

    struct epoll_event ev;
    struct epoll_event events[EPOLL_SIZE_MAX];
    epollfd = epoll_create(EPOLL_SIZE_MAX);
    ev.events = EPOLLIN;
    ev.data.fd = listenfd;
    epoll_ctl(epollfd, EPOLL_CTL_ADD, listenfd, &ev);

    printf("Server is running at port %d...\n", conf.port);

    int i, n;
    while (1) {
        n = epoll_wait(epollfd, events, events_count, -1);
        if (n == -1) {
            perror("epoll_wait");
            continue;
        }
        for (i = 0; i < n; ++i) {
            if (events[i].data.fd == listenfd) {
                if (events_count >= EPOLL_SIZE_MAX) {
                    perror("too many connection");
                } else {
                    handle_accept(epollfd, &events[i]);
                }
            } else if (events[i].events & EPOLLIN) {
                handle_in(epollfd, &events[i], &conf);
            } else if (events[i].events & EPOLLOUT) {
                handle_out(epollfd, &events[i], &conf);
            }
        }
    }
    shutdown(listenfd, SHUT_RDWR);
    return 0;
}

void unregister_event(int epollfd, struct epoll_event *ev)
{
    epoll_io_t *io_data = ev->data.ptr;
    int connfd = io_data->connfd;
    free(io_data);
    ev->data.ptr = 0;
    shutdown(connfd, SHUT_RDWR);
    epoll_ctl(epollfd, EPOLL_CTL_DEL, connfd, ev);
    events_count--;
}

int is_timeout(struct epoll_event *ev)
{
    epoll_io_t *io_data = ev->data.ptr;
    return io_data->timestamp != 0 &&
           io_data->timestamp + io_data->keepalive_time < time(0);
}

void reset_epoll(int epollfd, struct epoll_event *ev)
{
    epoll_io_t *io_data = ev->data.ptr;
    int connfd = io_data->connfd;
    if (io_data->readfd) {
        close(io_data->readfd);
        io_data->readfd = 0;
    }
    if (io_data->req) {
        free(io_data->req);
        io_data->req = 0;
    }
    if (io_data->buf) {
        free(io_data->buf);
        io_data->buf = 0;
    }
    io_data->timestamp = time(0);
    ev->events = EPOLLIN;
    epoll_ctl(epollfd, EPOLL_CTL_MOD, connfd, ev);
}

int handle_accept(int epollfd, struct epoll_event *ev)
{
    struct sockaddr_in client;
    int listenfd = ev->data.fd;
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
    if (setnonblocking(connfd) < 0) {
        perror("setnonblocking error");
    }
    io_data->connfd = connfd;
    ev->data.ptr = io_data;
    ev->events = EPOLLIN;
    epoll_ctl(epollfd, EPOLL_CTL_ADD, connfd, ev);
    events_count++;
    return 0;
}

int handle_in(int epollfd, struct epoll_event *ev, config_t *conf)
{
    epoll_io_t *io_data = ev->data.ptr;
    int connfd = io_data->connfd;
    char *buf = calloc(BUF_SIZE, sizeof(char));
    ssize_t n = recv(connfd, buf, BUF_SIZE, 0);
    if (n > 0) {
        http_req_t *req = calloc(1, sizeof(http_req_t));
        parse_http_req(req, buf, conf);
        io_data->connfd = connfd;
        io_data->req = req;
        if (!req->keep_alive) {
            io_data->keepalive_time = 0;
        } else {
            io_data->keepalive_time = conf->keepalive_time;
        }
        ev->events = EPOLLOUT;
        epoll_ctl(epollfd, EPOLL_CTL_MOD, connfd, ev);
    }
    free(buf);
    return 0;
}

void send_data(int epollfd, struct epoll_event *ev, int more)
{
    epoll_io_t *io_data = ev->data.ptr;
    int connfd = io_data->connfd;
    if (send(connfd, io_data->buf, io_data->size, 0) != -1) {
        free(io_data->buf);
        io_data->buf = 0;
        if (more) {
            epoll_ctl(epollfd, EPOLL_CTL_MOD, connfd, ev);
        } else {
            reset_epoll(epollfd, ev);
        }
    } else {
        if ((errno != EAGAIN) && (errno != EWOULDBLOCK)) {
            perror("send error");
            io_data->keepalive_time = 0;
            reset_epoll(epollfd, ev);
        }
    }
}

int handle_out(int epollfd, struct epoll_event *ev, config_t *conf)
{
    epoll_io_t *io_data = ev->data.ptr;
    if (io_data->buf != 0) {
        send_data(epollfd, ev, 0);
    } else {
        io_data->buf = calloc(BUF_SIZE, sizeof(char));
        int fd = io_data->readfd;
        if (fd == 0) {
            http_req_t *req = io_data->req;
            http_resp_header_t resp_header;
            bzero(&resp_header, sizeof(http_resp_header_t));

            resp_header.version = req->version;
            resp_header.keep_alive = req->keep_alive;
            if (req->error_code == 0) {
                strcpy(io_data->buf, conf->root);
                strcat(io_data->buf, req->uri);
                if ((fd = open(io_data->buf, O_RDONLY)) == -1) {
                    req->error_code = ERR_NOT_FOUND;
                }
            }
            if (req->error_code == 0) {
                if (setnonblocking(fd) < 0) {
                    perror("setnonblocking error");
                }
                resp_header.status_code = err_status_code[0];
                resp_header.content_length = size_of_file(io_data->buf);
                if (io_data->req->method == POST) {
                    resp_header.content_length += 29 + strlen(io_data->req->post_data);
                }
                strcpy(resp_header.content_type, req->content_type);
                bzero(io_data->buf, BUF_SIZE);
                build_http_resp_header(io_data->buf, &resp_header);
                io_data->readfd = fd;
                io_data->size = strlen(io_data->buf);
                send_data(epollfd, ev, 1);
            } else {
                resp_header.status_code = err_status_code[req->error_code];
                resp_header.content_length = strlen(err_resp[req->error_code]);
                strcpy(resp_header.content_type, "text/html");
                bzero(io_data->buf, BUF_SIZE);
                build_http_resp_header(io_data->buf, &resp_header);
                strcat(io_data->buf, err_resp[req->error_code]);
                io_data->size = strlen(io_data->buf);
                send_data(epollfd, ev, 0);
            }
        } else {
            bzero(io_data->buf, BUF_SIZE);
            size_t n = (size_t) read(fd, io_data->buf, BUF_SIZE);
            if (n > 0) {
                io_data->size = n;
                send_data(epollfd, ev, 1);
            } else if (n == 0) {
                if (io_data->req->method == POST && strcmp(io_data->req->content_type, "text/html") == 0) {
                    append_post_reponse(io_data->buf, io_data->req->post_data);
                    io_data->size = strlen(io_data->buf);
                    send_data(epollfd, ev, 0);
                } else {
                    reset_epoll(epollfd, ev);
                }
            }
        }
    }
    return 0;
}

int setnonblocking(int sockfd)
{
    if (fcntl(sockfd, F_SETFL, fcntl(sockfd, F_GETFL, 0) | O_NONBLOCK) == -1) {
        return -1;
    }
    return 0;
}
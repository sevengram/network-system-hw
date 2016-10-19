#ifndef NETWORKS_HTTP_H
#define NETWORKS_HTTP_H

#include "strutil.h"
#include "config.h"
#include <stdint.h>
#include <time.h>

#define BUF_SIZE        4096

#define GET     1
#define POST    2

#define ERR_BAD_REQUEST         1
#define ERR_NOT_FOUND           2
#define ERR_SERVER_ERROR        3
#define ERR_NOT_IMPLEMENTED     4

extern const uint16_t err_status_code[];

extern const char *err_resp[];

typedef struct
{
    uint8_t method;
    uint8_t version;
    uint8_t keep_alive;
    uint8_t error_code;
    char host[256];
    char uri[1024];
    char content_type[64];
    char post_data[2048];
} http_req_t;

typedef struct
{
    uint8_t version;
    uint16_t status_code;
    uint8_t keep_alive;
    int64_t content_length;
    char content_type[64];
} http_resp_header_t;

typedef struct
{
    int connfd;
    int readfd;
    time_t timestamp;
    uint32_t keepalive_time;
    http_req_t *req;
    char *buf;
    size_t size;
} epoll_io_t;

void parse_http_req(http_req_t *req, char *buf, config_t *conf);

void build_http_resp_header(char *buf, http_resp_header_t *resp_header);

void append_post_reponse(char *buf, char *post_data);

#endif //NETWORKS_HTTP_H

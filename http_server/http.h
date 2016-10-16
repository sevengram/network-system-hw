#ifndef NETWORKS_HTTP_H
#define NETWORKS_HTTP_H

#include "strutil.h"
#include <stdint.h>

#define GET     1
#define POST    2

#define ERR_REQ_INVALID_FORMAT      91
#define ERR_REQ_INVALID_METHOD      92
#define ERR_REQ_INVALID_VERSION     93

typedef struct
{
    uint8_t method;
    uint8_t version;
    uint8_t keep_alive;
    uint8_t error_code;
    char host[256];
    char uri[1024];
} http_req_t;

typedef struct
{
    uint8_t version;
    uint16_t status_code;
    uint8_t keep_alive;
    uint8_t content_type;
    size_t content_length;
} http_resp_header_t;

http_req_t parse_http_req(char *buf);

void build_http_resp_header(char *buf, http_resp_header_t *resp_header);


#endif //NETWORKS_HTTP_H

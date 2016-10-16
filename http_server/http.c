#include <stdio.h>
#include <string.h>
#include "http.h"
#include "util.h"

#define LINE_MAX 32

const char *http_resp_header_format = "HTTP/1.1 %d %s\r\nDate: %s\r\nContent-Length: %d\r\nConnection: %s\r\nContent-Type: %s\r\n\r\n";

http_req_t parse_http_req(char *buf)
{
    int i;
    char lines[LINE_MAX][LINE_LEN_MAX];
    int n = split(buf, "\n", lines);
    http_req_t result;
    bzero(&result, sizeof(result));
    if (n == 0) {
        result.error_code = ERR_REQ_INVALID_FORMAT;
        return result;
    }
    remove_endl(lines[0]);
    char p[4][LINE_LEN_MAX];
    int m = split(lines[0], " ", p);
    if (m < 3) {
        result.error_code = ERR_REQ_INVALID_FORMAT;
        return result;
    }
    if (strcmp(p[0], "GET") == 0) {
        result.method = GET;
    } else {
        result.error_code = ERR_REQ_INVALID_METHOD;
    }
    if (strcmp(p[2], "HTTP/1.1") == 0) {
        result.version = 11;
    } else if (strcmp(p[2], "HTTP/1.0") == 0) {
        result.version = 10;
    } else {
        result.error_code = ERR_REQ_INVALID_VERSION;
    }
    strcpy(result.uri, p[1]);
    for (i = 1; i < n; i++) {
        remove_endl(lines[i]);
        lower(lines[i]);
        split_first(lines[i], ':', p[0], p[1]);
        if (m >= 2) {
            strip(p[0]);
            if (strcmp(p[0], "host") == 0) {
                strip(p[1]);
                strcpy(result.host, p[1]);
            } else if (strcmp(p[0], "connection") == 0) {
                strip(p[1]);
                result.keep_alive = (uint8_t) ((strcmp(p[1], "keep-alive") == 0) ? 1 : 0);
            }
        }
    }
    return result;
}

const char *get_reason_phrase(uint16_t status_code)
{
    switch (status_code) {
        case 200:
            return "OK";
        case 404:
            return "Not Found";
        default:
            return "Internal Server Error";
    }
}

const char *get_http_version_str(uint8_t version)
{
    switch (version) {
        case 10:
            return "HTTP/1.0";
        case 11:
            return "HTTP/1.1";
        default:
            return "";
    }
}

const char *get_content_type_str(uint8_t content_type)
{
    return "text/html";
}

void build_http_resp_header(char *buf, http_resp_header_t *resp_header)
{
    char time_buf[128];
    format_current_time(time_buf, sizeof(time_buf));
    sprintf(buf, http_resp_header_format,
            resp_header->status_code,
            get_reason_phrase(resp_header->status_code),
            time_buf,
            resp_header->content_length,
            resp_header->keep_alive ? "keep-alive" : "close",
            get_content_type_str(resp_header->content_type));
}
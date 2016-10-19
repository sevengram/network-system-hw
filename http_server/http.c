#include <stdio.h>
#include <string.h>
#include "http.h"
#include "util.h"

const char *http_resp_header_format = "%s %d %s\r\nDate: %s\r\nContent-Length: %d\r\nConnection: %s\r\nContent-Type: %s\r\nCache-control: max-age=3600\r\n\r\n";

const uint16_t err_status_code[] = {
        200,
        400,
        404,
        500,
        501
};

const char *err_resp[] = {
        "",
        "<html><body>400 Bad Request</body></html>",
        "<html><body>404 Not Found</body></html>",
        "<html><body>500 Internal Server Error</body></html>",
        "<html><body>501 Not Implemented</body></html>"
};

void parse_http_req(http_req_t *req, char *buf, config_t *conf)
{
    int i, n;
    char **lines = malloc_strings(32, 256);
    char **p = malloc_strings(4, 128);

    bzero(req, sizeof(http_req_t));
    if ((n = split(buf, "\n", lines)) == 0) {
        req->error_code = ERR_BAD_REQUEST;
    }

    remove_endl(lines[0]);
    int m = split(lines[0], " ", p);
    if (m < 3) {
        req->error_code = ERR_BAD_REQUEST;
    } else {
        if (strcmp(p[0], "GET") == 0) {
            req->method = GET;
        } else if (strcmp(p[0], "POST") == 0) {
            req->method = POST;
        } else {
            req->error_code = ERR_NOT_IMPLEMENTED;
        }
        strcpy(req->uri, p[1]);
        if (strcmp(req->uri, "/") == 0) {
            strcat(req->uri, conf->index[0]);
        }
        get_content_type(conf, req->uri, req->content_type);
        if (strlen(req->content_type) == 0) {
            req->error_code = ERR_NOT_IMPLEMENTED;
        }
        if (strcmp(p[2], "HTTP/1.1") == 0) {
            req->version = 11;
        } else if (strcmp(p[2], "HTTP/1.0") == 0) {
            req->version = 10;
        } else {
            req->error_code = ERR_BAD_REQUEST;
        }
        for (i = 1; i < n; i++) {
            remove_endl(lines[i]);
            if (strlen(lines[i]) == 0) {
                break;
            }
            lower(lines[i]);
            split_first(lines[i], ':', p[0], p[1]);
            strip(p[0], ' ');
            if (strcmp(p[0], "host") == 0) {
                strip(p[1], ' ');
                strcpy(req->host, p[1]);
            } else if (strcmp(p[0], "connection") == 0) {
                strip(p[1], ' ');
                req->keep_alive = (uint8_t) ((strcmp(p[1], "keep-alive") == 0) ? 1 : 0);
            }
        }
        for (i = i + 1; i < n; i++) {
            strcat(req->post_data, lines[i]);
            if (i != n - 1) {
                strcat(req->post_data, "\n");
            }
        }
    }
    free_strings(p, 4);
    free_strings(lines, 32);
}

const char *get_reason_phrase(uint16_t status_code)
{
    switch (status_code) {
        case 200:
            return "OK";
        case 400:
            return "Bad Request";
        case 404:
            return "Not Found";
        case 501:
            return "Not Implemented";
        case 500:
            return "Internal Server Error";
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

void build_http_resp_header(char *buf, http_resp_header_t *resp_header)
{
    char time_buf[128];
    bzero(time_buf, sizeof(time_buf));
    format_current_time(time_buf, sizeof(time_buf));
    sprintf(buf, http_resp_header_format,
            get_http_version_str(resp_header->version),
            resp_header->status_code,
            get_reason_phrase(resp_header->status_code),
            time_buf,
            resp_header->content_length,
            resp_header->keep_alive ? "keep-alive" : "close",
            resp_header->content_type);
}

void append_post_reponse(char *buf, char *post_data)
{
    char tmp[1024];
    sprintf(tmp, "<h1>Post Data</h1><pre>%s</pre>", post_data);
    strcat(buf, tmp);
}
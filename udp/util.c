#include <string.h>
#include <dirent.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include "util.h"

int is_endl(char *line)
{
    return strlen(line) == 1 && line[0] == '\n';
}

void remove_endl(char *line)
{
    size_t n = strlen(line);
    if (n > 0 && line[n - 1] == '\n') {
        line[n - 1] = '\0';
    }
}

void strip(char *line)
{
    // TODO
}

int parse_line(char *cmdline, char argv[][ARG_LEN_MAX])
{
    char *buffer = malloc(LINE_MAX);
    char *p;
    int i = 0;
    strcpy(buffer, cmdline);
    strip(buffer);
    remove_endl(buffer);
    if (strlen(buffer) == 0) {
        return 0;
    }
    while ((p = strsep(&buffer, " ")) != NULL) {
        strcpy(argv[i], p);
        i++;
    }
    free(buffer);
    return i;
}

void list_dir(char *buffer, const char *name, char delimiter)
{
    struct dirent *dir;
    DIR *d = opendir(name);
    size_t pos = 0;
    if (d) {
        while ((dir = readdir(d)) != NULL) {
            if (strcmp(dir->d_name, ".") && strcmp(dir->d_name, "..")) {
                sprintf(buffer + pos, "%s%c", dir->d_name, delimiter);
                pos = strlen(buffer);
            }
        }
        closedir(d);
    }
    buffer[pos] = '\0';
}

void build_data_packet(data_packet_t *pkt, uint64_t _offset, char *_data, uint16_t _data_len)
{
    memset(pkt, 0, PACKET_SIZE);
    pkt->header.pkt_type = DATA_TYPE;
    if (_data) {
        pkt->offset = _offset;
        pkt->data_len = _data_len;
        memcpy(pkt->data, _data, _data_len);
    }
}

void build_msg_packet(msg_packet_t *pkt, uint8_t _req_type, char *_msg, uint16_t _msg_len)
{
    memset(pkt, 0, PACKET_SIZE);
    pkt->header.pkt_type = MSG_TYPE;
    pkt->req_type = _req_type;
    if (_msg) {
        pkt->msg_len = _msg_len;
        memcpy(pkt->msg, _msg, _msg_len);
    }
}

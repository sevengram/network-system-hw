#include <string.h>
#include <dirent.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/socket.h>
#include "transmission.h"
#include "util.h"

/*
 * List the files in the directory, save the result in buffer
 */
void list_dir(char *buffer, const char *dir_name, char delimiter)
{
    struct dirent *dir;
    DIR *d = opendir(dir_name);
    size_t pos = 0;
    if (d) {
        while ((dir = readdir(d)) != NULL) {
            if (strcmp(dir->d_name, ".") && strcmp(dir->d_name, "..") && dir->d_type != DT_DIR) {
                sprintf(buffer + pos, "%s%c", dir->d_name, delimiter);
                pos = strlen(buffer);
            }
        }
        closedir(d);
    }
    buffer[pos] = '\0';
}

void build_msg_packet(msg_packet_t *pkt, uint8_t _msg_type, uint8_t _errno, char *_msg, uint16_t _msg_len,
                      client_config_t *conf)
{
    memset(pkt, 0, PACKET_SIZE);
    pkt->header.pkt_type = MSG_TYPE;
    if (conf != 0) {
        strcpy(pkt->header.username, conf->username);
        strcpy(pkt->header.password, conf->password);
    }
    pkt->msg_type = _msg_type;
    pkt->errno = _errno;
    if (_msg) {
        memcpy(pkt->msg, _msg, _msg_len);
    }
}

void build_data_packet(data_packet_t *pkt, uint32_t _offset, char *_data, uint16_t _data_len, uint16_t eof)
{
    memset(pkt, 0, PACKET_SIZE);
    pkt->header.pkt_type = DATA_TYPE;
    pkt->eof = eof;
    if (_data) {
        pkt->offset = _offset;
        pkt->data_len = _data_len;
        memcpy(pkt->data, _data, _data_len);
    }
}

int send_file(FILE *fp, long offset, long n, int sock, const char *key)
{
    if (fp == NULL) {
        return -1;
    }
    char data[DATA_SIZE_MAX];
    size_t nbytes;
    uint32_t c = 0;
    data_packet_t *outpkt = malloc(PACKET_SIZE);
    fseek(fp, offset, SEEK_SET);
    int i;
    size_t l = (key != 0) ? strlen(key) : 0;
    while (!feof(fp) && (n == -1 || n > 0)) {
        nbytes = fread(data, sizeof(char), (n != -1) ? MIN(n, DATA_SIZE_MAX) : DATA_SIZE_MAX, fp);
        if (key != 0) {
            for (i = 0; i < nbytes; i++) {
                data[i] = data[i] ^ key[i % l];
            }
        }
        build_data_packet(outpkt, c, data, (uint16_t) nbytes, 0);
        if (send(sock, outpkt, PACKET_SIZE, 0) <= 0) {
            return -1;
        }
        c += nbytes;
        if (n != -1) {
            n -= nbytes;
        }
    }
    build_data_packet(outpkt, 0, NULL, 0, 1);
    send(sock, outpkt, PACKET_SIZE, 0);
    free(outpkt);
    return 0;
}

int receive_file(FILE *fp, int sock, const char *key)
{
    if (fp == NULL) {
        return -1;
    }
    int i;
    char buffer[PACKET_SIZE];
    uint64_t pos = 0;
    data_packet_t *dpkt;
    size_t l = (key != 0) ? strlen(key) : 0;
    while (1) {
        memset(buffer, 0, sizeof(buffer));
        if (recv(sock, buffer, sizeof(buffer), 0) <= 0) {
            return -1;
        }
        dpkt = (data_packet_t *) buffer;
        if (dpkt->eof == 1) {
            break;
        }
        if (dpkt->offset != pos) {
            pos = dpkt->offset;
            fseek(fp, pos, SEEK_SET);
        }
        if (key != 0) {
            for (i = 0; i < dpkt->data_len; i++) {
                dpkt->data[i] = dpkt->data[i] ^ key[i % l];
            }
        }
        fwrite(dpkt->data, sizeof(char), dpkt->data_len, fp);
        pos += dpkt->data_len;
    }
    return 0;
}
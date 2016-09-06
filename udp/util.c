#include <string.h>
#include <dirent.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/socket.h>
#include "util.h"

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

/*
 * Parse the command line, save the result in argv
 */
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
            if (strcmp(dir->d_name, ".") && strcmp(dir->d_name, "..")) {
                sprintf(buffer + pos, "%s%c", dir->d_name, delimiter);
                pos = strlen(buffer);
            }
        }
        closedir(d);
    }
    buffer[pos] = '\0';
}

void build_data_packet(data_packet_t *pkt, uint32_t _offset, char *_data, uint16_t _data_len)
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
    pkt->msg_type = _req_type;
    if (_msg) {
        pkt->msg_len = _msg_len;
        memcpy(pkt->msg, _msg, _msg_len);
    }
}

void set_received_filename(char *filename, char *path)
{
    char *p = strrchr(path, '/');
    if (!p) {
        p = path;
    } else {
        p = p + 1;
    }
    sprintf(filename, "%s.received", p);
}

void send_file(FILE *fp, int sock, const struct sockaddr *addr, socklen_t addr_len)
{
    if (fp == NULL) {
        return;
    }
    char data[DATA_SIZE_MAX];
    size_t nbytes;
    uint32_t offset = 0;
    data_packet_t *outpkt = malloc(PACKET_SIZE);
    while (!feof(fp)) {
        nbytes = fread(data, sizeof(char), DATA_SIZE_MAX, fp);
        build_data_packet(outpkt, offset, data, (uint16_t) nbytes);
        sendto(sock, outpkt, PACKET_SIZE, 0, addr, addr_len);
        sendto(sock, outpkt, PACKET_SIZE, 0, addr, addr_len);
        offset += nbytes;
    }
    build_data_packet(outpkt, offset, NULL, 0);
    sendto(sock, outpkt, PACKET_SIZE, 0, addr, addr_len);
    free(outpkt);
}

void receive_file(FILE *fp, int sock, struct sockaddr *addr, socklen_t *addr_len)
{
    if (fp == NULL) {
        return;
    }
    char buffer[PACKET_SIZE];
    uint64_t pos = 0;
    data_packet_t *dpkt;
    while (1) {
        memset(buffer, 0, sizeof(buffer));
        recvfrom(sock, buffer, sizeof(buffer), 0, addr, addr_len);
        dpkt = (data_packet_t *) buffer;
        if (dpkt->data_len == 0) {
            break;
        }
        if (dpkt->offset != pos) {
            pos = dpkt->offset;
            fseek(fp, pos, SEEK_SET);
        }
        fwrite(dpkt->data, sizeof(char), dpkt->data_len, fp);
        pos += dpkt->data_len;
    }
    fclose(fp);
}
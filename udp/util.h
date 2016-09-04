#ifndef NETWORKS_UTIL_H
#define NETWORKS_UTIL_H

#include "stdint.h"

#define MSG_TYPE     1
#define DATA_TYPE    2

#define CMD_LS       1
#define CMD_PUT      2
#define CMD_GET      3
#define CMD_EXIT     4
#define CMD_UND      5

#define PACKET_SIZE         2048
#define HEADER_SIZE         2
#define BODY_SIZE           (PACKET_SIZE - HEADER_SIZE)
#define DATA_SIZE_MAX       (BODY_SIZE - 8 - 2)
#define MSG_SIZE_MAX        1024

#define BUF_SIZE_MAX        1024
#define ARG_MAX             16
#define ARG_LEN_MAX         256
#define LINE_MAX            1024  /* max line size */


typedef struct
{
    uint8_t empty;
    uint8_t pkt_type;
} header_t;

typedef struct
{
    header_t header;
    char body[BODY_SIZE];
} packet_t;

typedef struct
{
    header_t header;
    uint8_t empty;
    uint8_t req_type;
    uint16_t msg_len;
    char msg[MSG_SIZE_MAX];
    char padding[BODY_SIZE - 4 - MSG_SIZE_MAX];
} msg_packet_t;

typedef struct
{
    header_t header;
    uint64_t offset;
    uint16_t data_len;
    char data[DATA_SIZE_MAX];
} data_packet_t;

void build_data_packet(data_packet_t *pkt, uint64_t _offset, char *_data, uint16_t _data_len);

void build_msg_packet(msg_packet_t *pkt, uint8_t _req_type, char *_msg, uint16_t _msg_len);

int parse_line(char *cmdline, char argv[][ARG_LEN_MAX]);

void remove_endl(char *line);

int is_endl(char *line);

void list_dir(char *buffer, const char *name, char delimiter);

#endif //NETWORKS_UTIL_H

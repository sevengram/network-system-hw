#ifndef NETWORKS_UTIL_H
#define NETWORKS_UTIL_H

#include <stdint.h>

#define MSG_TYPE     1
#define DATA_TYPE    2

#define PACKET_SIZE         496
#define HEADER_SIZE         2
#define BODY_SIZE           (PACKET_SIZE - HEADER_SIZE)
#define DATA_SIZE_MAX       400
#define MSG_SIZE_MAX        256

typedef struct
{
    uint8_t empty;          /* padding */
    uint8_t pkt_type;       /* type of packet: MSG_TYPE or DATA_TYPE */
} header_t;

typedef struct
{
    header_t header;
    char body[BODY_SIZE];
} packet_t;

typedef struct
{
    header_t header;
    uint8_t errno;          /* errno */
    uint8_t msg_type;       /* type of message: CMD_LS, CMD_GET, CMD_PUT, etc. */
    uint16_t msg_len;       /* length of message */
    char msg[MSG_SIZE_MAX]; /* message */
    char padding[BODY_SIZE - 4 - MSG_SIZE_MAX];  /* padding */
} msg_packet_t;

typedef struct
{
    header_t header;
    uint16_t eof;           /* end of file */
    uint16_t data_len;      /* length of data */
    uint32_t offset;        /* offset */
    char data[DATA_SIZE_MAX];   /* data */
    char padding[BODY_SIZE - 8 - DATA_SIZE_MAX];  /* padding */
} data_packet_t;

void build_data_packet(data_packet_t *pkt, uint32_t _offset, char *_data, uint16_t _data_len, uint16_t eof);

void build_msg_packet(msg_packet_t *pkt, uint8_t _msg_type, uint8_t _errno, char *_msg, uint16_t _msg_len);

void list_dir(char *buffer, const char *dir_name, char delimiter);

void send_file(FILE *fp, long offset, long n, int sock);

void receive_file(FILE *fp, int sock);


#endif //NETWORKS_UTIL_H

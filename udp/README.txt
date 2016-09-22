Programming Assignment 1

Jianxiang Fan
jianxiang.fan@colorado.edu


> How to run the program

$ make clean
$ make all
$ ./server 6000
$ ./client 127.0.0.1 6000


> Commands for client-end

ls : List the files in the server's local directory

get [file_name]: The server transmits the requested file to the client. An error message prints if some problems occur.

put [file_name]: The server receives the transmitted file by the client and stores it locally. An error message prints
if some problems occur.

exit: The server exits.

Any other commands: The server echoes back.


> Data Struture

Two types of packet is defined in util.h

typedef struct
{
    header_t header;
    uint8_t empty;          /* padding */
    uint8_t msg_type;       /* type of message: CMD_LS, CMD_GET, CMD_PUT, etc. */
    uint16_t msg_len;       /* length of message */
    char msg[MSG_SIZE_MAX]; /* message */
    char padding[BODY_SIZE - 4 - MSG_SIZE_MAX];  /* padding */
} msg_packet_t;

typedef struct
{
    header_t header;
    uint16_t data_len;      /* length of data */
    uint32_t offset;        /* offset */
    char data[DATA_SIZE_MAX];   /* data */
    char padding[BODY_SIZE - 6 - DATA_SIZE_MAX];  /* padding */
} data_packet_t;

message packet is used to transfer commands from the client-end and the message responses from the server-end.

data packet is used to transfer the file. Offset is used to indicates the position of the data for reconstructing
the file.

> File Transfer (GET/PUT)

First, the client send a message packet indicates the command (GET/PUT) and its operand.

If no problem (file not found, unable to create the file, etc.) occurs, the server sends back a message packet
(GET_START/PUT_START) which means that the sever is ready send/receive the file.

When the client receives the GET_START/PUT_START message, it starts to receive/send the file. The received file
will be named as *.received

Redundancy is adopted to increase the reliabilty. Each data packet will be sent with a duplicate.

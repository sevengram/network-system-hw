#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include "util.h"

int main(int argc, char *argv[])
{
    int sock;                           //This will be our socket
    struct sockaddr_in local, remote;   //"Internet socket address structure"
    socklen_t addr_len;                 //length of the sockaddr_in structure

    if (argc != 2) {
        printf("USAGE: <port>\n");
        exit(1);
    }

    addr_len = sizeof(struct sockaddr);

    /*
     * This code populates the sockaddr_in struct with
     * the information about our socket
     */
    memset(&local, 0, sizeof(local));                 //zero the struct
    local.sin_family = AF_INET;                   //address family
    local.sin_port = htons(atoi(argv[1]));        //htons() sets the port # to network byte order
    local.sin_addr.s_addr = INADDR_ANY;           //supplies the IP address of the local machine

    // Causes the system to create a generic socket of type UDP (datagram)
    if ((sock = socket(PF_INET, SOCK_DGRAM, 0)) < 0) {
        printf("unable to create socket");
        exit(1);
    }

    // Eliminates "Address already in use" error from bind.
    int optval = 1;
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (const void *) &optval, sizeof(int)) < 0) {
        printf("unable to call setsockopt");
        exit(1);
    }

    /*
     * Once we've created a socket, we must bind that socket to the
     * local address and port we've supplied in the sockaddr_in struct
     */
    if (bind(sock, (struct sockaddr *) &local, addr_len) < 0) {
        printf("unable to bind socket\n");
    }

    int exit_flag = 0;
    char buffer[PACKET_SIZE];
    char data[DATA_SIZE_MAX];
    char message[LINE_MAX];
    packet_t *outpkt = malloc(PACKET_SIZE);
    while (1) {
        memset(buffer, 0, sizeof(buffer));
        recvfrom(sock, buffer, sizeof(buffer), 0, (struct sockaddr *) &remote, &addr_len);

        if (((packet_t *) buffer)->header.pkt_type == MSG_TYPE) {
            msg_packet_t *mpkt = (msg_packet_t *) buffer;
            switch (mpkt->req_type) {
                case CMD_LS:
                    list_dir(message, ".", '\n');
                    build_msg_packet((msg_packet_t *) outpkt, mpkt->req_type, message, (uint16_t) strlen(message));
                    break;
                case CMD_EXIT:
                    strcpy(message, "Server will exit.\n");
                    build_msg_packet((msg_packet_t *) outpkt, mpkt->req_type, message, (uint16_t) strlen(message));
                    exit_flag = 1;
                    break;
                case CMD_GET:
                    if (strlen(mpkt->msg) > 0) {
                        FILE *fp;
                        size_t nbytes;
                        if ((fp = fopen(mpkt->msg, "rb")) != NULL) {
                            while (!feof(fp)) {
                                nbytes = fread(data, sizeof(char), DATA_SIZE_MAX, fp);
                                // TODO: change to data
                                build_msg_packet((msg_packet_t *) outpkt, mpkt->req_type, data, (uint16_t) nbytes);
                                sendto(sock, outpkt, nbytes, 0, (struct sockaddr *) &remote, addr_len);
                            }
                        } else {
                            // TODO
                        }
                    } else {
                        // TODO
                    }
                default:
                    strcpy(message, mpkt->msg);
                    build_msg_packet((msg_packet_t *) outpkt, mpkt->req_type, message, (uint16_t) strlen(message));
            }
        } else {
            // TODO: data packet
        }
        sendto(sock, outpkt, PACKET_SIZE, 0, (struct sockaddr *) &remote, addr_len);
        if (exit_flag == 1) {
            break;
        }
    }

    free(outpkt);
    close(sock);
    return 0;
}


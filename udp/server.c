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
    if (argc != 2) {
        printf("USAGE: <port>\n");
        exit(1);
    }

    int sock;                           // This will be our socket
    struct sockaddr_in local, remote;   // "Internet socket address structure"
    socklen_t addr_len = sizeof(struct sockaddr);   // length of the sockaddr_in structure

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
    char msg[LINE_LEN_MAX];
    uint8_t cmd_type;
    packet_t *outpkt = malloc(PACKET_SIZE);
    FILE *fp = NULL;
    char filename[FILENAME_MAX];
    while (1) {
        cmd_type = MSG_RESP;
        memset(msg, 0, sizeof(msg));
        memset(buffer, 0, sizeof(buffer));
        recvfrom(sock, buffer, sizeof(buffer), 0, (struct sockaddr *) &remote, &addr_len);
        msg_packet_t *mpkt = (msg_packet_t *) buffer;
        switch (mpkt->msg_type) {
            case CMD_LS:
                list_dir(msg, ".", '\n');
                break;
            case CMD_EXIT:
                strcpy(msg, "Server will exit.\n");
                exit_flag = 1;
                break;
            case CMD_GET:
                if (strlen(mpkt->msg) > 0) {
                    if ((fp = fopen(mpkt->msg, "rb")) != NULL) {
                        cmd_type = GET_START;
                    } else {
                        strcpy(msg, "server: get: Cannot open the file\n");
                    }
                } else {
                    strcpy(msg, "server: get: Missing file operand\n");
                }
                break;
            case CMD_PUT:
                if (strlen(mpkt->msg) > 0) {
                    set_received_filename(filename, mpkt->msg);
                    if ((fp = fopen(filename, "wb")) != NULL) {
                        cmd_type = PUT_START;
                    } else {
                        strcpy(msg, "server: put: Cannot create the file\n");
                    }
                } else {
                    strcpy(msg, "server: get: Missing file operand\n");
                }
                break;
            default:
                strcpy(msg, mpkt->msg);
        }
        build_msg_packet((msg_packet_t *) outpkt, cmd_type, msg, (uint16_t) strlen(msg));
        sendto(sock, outpkt, PACKET_SIZE, 0, (struct sockaddr *) &remote, addr_len);

        if (cmd_type == GET_START && fp) {
            send_file(fp, sock, (struct sockaddr *) &remote, addr_len);
        } else if (cmd_type == PUT_START && fp) {
            receive_file(fp, sock, (struct sockaddr *) &remote, &addr_len);
        }
        if (fp != NULL) {
            fclose(fp);
            fp = NULL;
        }
        if (exit_flag == 1) {
            break;
        }
    }

    free(outpkt);
    close(sock);
    return 0;
}


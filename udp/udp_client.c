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
    if (argc < 3) {
        printf("USAGE: <server_ip> <server_port>\n");
        exit(1);
    }

    int sock;                            // this will be our socket
    struct sockaddr_in remote;           // "Internet socket address structure"
    socklen_t addr_len = sizeof(struct sockaddr); // length of the sockaddr_in structure

    /*
     * Here we populate a sockaddr_in struct with
     * information regarding where we'd like to send our packet
     * i.e the Server.
     */
    bzero(&remote, sizeof(remote));              //zero the struct
    remote.sin_family = AF_INET;                 //address family
    remote.sin_port = htons(atoi(argv[2]));      //sets port to network byte order
    remote.sin_addr.s_addr = inet_addr(argv[1]); //sets remote IP address

    // Causes the system to create a generic socket of type UDP (datagram)
    if ((sock = socket(PF_INET, SOCK_DGRAM, 0)) < 0) {
        printf("unable to create socket");
        exit(1);
    }

    /*
     * sendto() sends immediately.
     * it will report an error if the message fails to leave the computer
     * however, with UDP, there is no error if the message is lost in the network
     * once it leaves the AF_INET computer.
     */
    int _argc;
    char _argv[ARG_MAX][ARG_LEN_MAX];
    char cmdline[LINE_MAX];
    char buffer[PACKET_SIZE];
    msg_packet_t *outpkt = malloc(PACKET_SIZE);
    FILE *fp = NULL;
    uint8_t cmd_type = CMD_UND;
    char *msg = NULL;
    char filename[FILENAME_MAX];
    while (1) {
        printf("> ");
        if (fgets(cmdline, LINE_MAX, stdin) != NULL) {
            _argc = parse_line(cmdline, _argv);
            if (_argc == 0) {
                continue;
            } else if (_argc > 0 && strcmp(_argv[0], "exit") == 0) {
                cmd_type = CMD_EXIT;
            } else if (_argc > 0 && strcmp(_argv[0], "ls") == 0) {
                cmd_type = CMD_LS;
            } else if (_argc > 0 && strcmp(_argv[0], "get") == 0) {
                if (_argc > 1) {
                    set_received_filename(filename, _argv[1]);
                    if ((fp = fopen(filename, "wb")) != NULL) {
                        cmd_type = CMD_GET;
                        msg = _argv[1];
                    } else {
                        // TODO
                    }
                } else {
                    // TODO
                }
            } else if (_argc > 0 && strcmp(_argv[0], "put") == 0) {
                if (_argc > 1) {
                    if ((fp = fopen(_argv[1], "rb")) != NULL) {
                        cmd_type = CMD_PUT;
                        msg = _argv[1];
                    } else {
                        // TODO
                    }
                } else {
                    // TODO
                }
            } else {
                cmd_type = CMD_UND;
                msg = cmdline;
            }
            build_msg_packet(outpkt, cmd_type, msg, (uint16_t) ((msg == NULL) ? 0 : strlen(msg)));
            sendto(sock, outpkt, PACKET_SIZE, 0, (struct sockaddr *) &remote, addr_len);
            memset(buffer, 0, sizeof(buffer));
            recvfrom(sock, buffer, sizeof(buffer), 0, (struct sockaddr *) &remote, &addr_len);

            msg_packet_t *mpkt = (msg_packet_t *) buffer;
            if (mpkt->msg_type == MSG_RESP) {
                printf("%s", mpkt->msg);
            } else if (mpkt->msg_type == GET_START && fp) {
                receive_file(fp, sock, (struct sockaddr *) &remote, &addr_len);
                fclose(fp);
            } else if (mpkt->msg_type == PUT_START && fp) {
                send_file(fp, sock, (struct sockaddr *) &remote, addr_len);
                fclose(fp);
            }
        }
    }

    free(outpkt);
    close(sock);
    return 0;
}


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
    int sock;                            //this will be our socket
    struct sockaddr_in remote;           //"Internet socket address structure"
    socklen_t addr_len;                  //length of the sockaddr_in structure
    char buffer[BUF_SIZE_MAX];

    if (argc < 3) {
        printf("USAGE: <server_ip> <server_port>\n");
        exit(1);
    }

    addr_len = sizeof(struct sockaddr);

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
    msg_packet_t *inpkt = malloc(PACKET_SIZE);
    while (1) {
        printf("> ");
        if (fgets(cmdline, LINE_MAX, stdin) != NULL) {
            _argc = parse_line(cmdline, _argv);
            if (_argc == 0) {
                continue;
            } else if (_argc > 0 && strcmp(_argv[0], "exit") == 0) {
                build_msg_packet(inpkt, CMD_EXIT, NULL, 0);
            } else if (_argc > 0 && strcmp(_argv[0], "ls") == 0) {
                build_msg_packet(inpkt, CMD_LS, NULL, 0);
            } else if (_argc > 0 && strcmp(_argv[0], "get") == 0) {
                if (_argc > 1) {
                    build_msg_packet(inpkt, CMD_GET, _argv[1], sizeof(_argv[1]));
                } else {
                    // TODO
                }
            } else {
                build_msg_packet(inpkt, CMD_UND, cmdline, sizeof(cmdline));
            }
            sendto(sock, inpkt, PACKET_SIZE, 0, (struct sockaddr *) &remote, addr_len);
            memset(buffer, 0, sizeof(buffer));
            recvfrom(sock, buffer, sizeof(buffer), 0, (struct sockaddr *) &remote, &addr_len);

            if (((packet_t *) buffer)->header.pkt_type == MSG_TYPE) {
                msg_packet_t *mpkt = (msg_packet_t *) buffer;
                printf("%s", mpkt->msg);
            }
        }
    }

    free(inpkt);
    close(sock);
    return 0;
}


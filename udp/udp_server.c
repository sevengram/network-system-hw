#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include "util.h"

#define MAXLINE     1024
#define MAXBUFSIZE  1024
#define MAXARGS     32

int main(int argc, char *argv[])
{
    int sock;                           //This will be our socket
    struct sockaddr_in local, remote;   //"Internet socket address structure"
    socklen_t addr_len;                 //length of the sockaddr_in structure
    char msg_buffer[MAXBUFSIZE];
    char resp_buffer[MAXBUFSIZE];

    if (argc != 2) {
        printf("USAGE: <port>\n");
        exit(1);
    }

    addr_len = sizeof(struct sockaddr);

    /*
     * This code populates the sockaddr_in struct with
     * the information about our socket
     */
    bzero(&local, sizeof(local));                 //zero the struct
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

    int _argc;
    char *_argv[MAXARGS];
    int exit_flag = 0;
    char cmdline[MAXLINE];
    while (1) {
        bzero(msg_buffer, sizeof(msg_buffer));
        bzero(resp_buffer, sizeof(resp_buffer));
        bzero(_argv, sizeof(char *) * MAXARGS);

        // waits for an incoming msg
        recvfrom(sock, msg_buffer, sizeof(msg_buffer), 0, (struct sockaddr *) &remote, &addr_len);
        strcpy(cmdline, msg_buffer);

        _argc = parse_line(msg_buffer, _argv);
        if (_argc > 0 && strcmp(_argv[0], "exit") == 0) {
            strcpy(resp_buffer, "Server will exit.\n");
            exit_flag = 1;
        } else if (_argc > 0 && strcmp(_argv[0], "ls") == 0) {
            list_dir(resp_buffer, ".", '\n');
        } else {
            strcpy(resp_buffer, cmdline);
        }

        sendto(sock, resp_buffer, strlen(resp_buffer), 0, (struct sockaddr *) &remote, addr_len);
        if (exit_flag == 1) {
            break;
        }
    }

    close(sock);
    return 0;
}


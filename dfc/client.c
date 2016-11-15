#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <netdb.h>
#include <pthread.h>
#include "transmission.h"
#include "strutil.h"
#include "util.h"

char **_argv;

msg_packet_t *shared_pkt;

int socks[32];

void *connection_handler(void *socket_desc);

/*
 * open_clientfd - open connection to server at <hostname, port>
 *   and return a socket descriptor ready for reading and writing.
 *   Returns -1 and sets errno on Unix error.
 *   Returns -2 and sets h_errno on DNS (gethostbyname) error.
 */
int open_clientfd(char *hostname, uint16_t port)
{
    int clientfd;
    struct hostent *hp;
    struct sockaddr_in serveraddr;

    if ((clientfd = socket(PF_INET, SOCK_STREAM, 0)) < 0)
        return -1; /* check errno for cause of error */

    /* Fill in the server's IP address and port */
    if ((hp = gethostbyname(hostname)) == NULL)
        return -2; /* check h_errno for cause of error */

    bzero((char *) &serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_port = htons(port);
    bcopy(hp->h_addr_list[0], (char *) &serveraddr.sin_addr.s_addr, hp->h_length);

    /* Establish a connection with the server */
    if (connect(clientfd, (struct sockaddr *) &serveraddr, sizeof(serveraddr)) < 0)
        return -1;
    return clientfd;
}

int main(int argc, char *argv[])
{
    int ns; // number of servers
    pthread_t threads[32];
    int i;

    ns = 4;
    for (i = 0; i < ns; i++) {
        if ((socks[i] = open_clientfd("127.0.0.1", 10001 + i)) < 0) {
            perror("unable to connect server");
            exit(1);
        }
    }

    int _argc;
    uint8_t cmd_type;
    char *msg = 0;
    char *cmdline = calloc(1024, sizeof(char));
    _argv = malloc_strings(16, 256);
    shared_pkt = calloc(1, sizeof(msg_packet_t));
    while (1) {
        printf("> ");
        if (fgets(cmdline, 1024, stdin) != NULL) {
            remove_endl(cmdline);
            strip(cmdline, ' ');
            _argc = split(cmdline, " ", _argv);
            if (_argc == 0) {
                continue;
            } else if (_argc > 0 && strcmp(_argv[0], "ls") == 0) {
                cmd_type = CMD_LS;
            } else if (_argc > 0 && strcmp(_argv[0], "get") == 0) {
                if (_argc > 1) {
                    cmd_type = CMD_GET;
                    msg = _argv[1];
                } else {
                    printf("get: Missing file operand\n");
                    continue;
                }
            } else if (_argc > 0 && strcmp(_argv[0], "put") == 0) {
                if (_argc > 1) {
                    cmd_type = CMD_PUT;
                    msg = _argv[1];
                } else {
                    printf("put: Missing file operand\n");
                    continue;
                }
            } else {
                cmd_type = 0;
                msg = cmdline;
            }
            build_msg_packet(shared_pkt, cmd_type, 0, msg, (uint16_t) ((msg == NULL) ? 0 : strlen(msg)));
            for (i = 0; i < ns; i++) {
                int *pi = malloc(1);
                *pi = i;
                if (pthread_create(threads + i, NULL, connection_handler, pi) < 0) {
                    perror("could not create thread");
                }
            }
            for (i = 0; i < ns; i++) {
                if (pthread_join(threads[i], NULL) < 0) {
                    perror("could not join thread");
                }
            }
        }
    }
    free_strings(_argv, 16);
    free(cmdline);
    free(shared_pkt);
    for (i = 0; i < ns; i++) {
        close(socks[i]);
    }
    return 0;
}

void *connection_handler(void *pindex)
{
    long fsize, psize;
    FILE *fp = 0;
    char *buffer = calloc(PACKET_SIZE, sizeof(char));
    msg_packet_t *outpkt = calloc(1, sizeof(msg_packet_t));
    bcopy(shared_pkt, outpkt, sizeof(msg_packet_t));
    int i = *(int *) pindex;
    if (outpkt->msg_type == CMD_GET || outpkt->msg_type == CMD_PUT) {
        get_partition_name(buffer, outpkt->msg, i);
        bzero(outpkt->msg, MSG_SIZE_MAX);
        strcpy(outpkt->msg, buffer);
    }
    if (send(socks[i], outpkt, PACKET_SIZE, 0) < 0) {
        perror("fail to send");
        return 0;
    } else {
        bzero(buffer, PACKET_SIZE);
        if (recv(socks[i], buffer, PACKET_SIZE, 0) < 0) {
            perror("fail to recv");
            return 0;
        } else {
            msg_packet_t *mpkt = (msg_packet_t *) buffer;
            if (strlen(mpkt->msg) != 0) {
                printf("%s\n", mpkt->msg);
            }
            if (mpkt->msg_type == CMD_GET && mpkt->errno == 0) {
                if ((fp = fopen(_argv[1], "wb")) == 0) {
                    perror("cannot create the file");
                    return 0;
                }
                receive_file(fp, socks[i]);
            } else if (mpkt->msg_type == CMD_PUT && mpkt->errno == 0) {
                if ((fp = fopen(_argv[1], "rb")) == 0) {
                    perror("cannot open the file");
                    return 0;
                }
                fsize = size_of_fp(fp);
                psize = (fsize + 1) / 4;
                send_file(fp, psize * i, psize, socks[i]);
            }
        }
    }
    if (fp != 0) {
        fclose(fp);
    }
    free(pindex);
    free(buffer);
    return 0;
}

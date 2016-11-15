/*
    C socket server example, handles multiple clients using threads
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <unistd.h>
#include "transmission.h"
#include "util.h"

#define LISTENQ         1024

void *connection_handler(void *);

int main(int argc, char *argv[])
{
    int socket_desc, client_sock, *new_sock;
    socklen_t socklen;
    struct sockaddr_in server, client;

    chdir(argv[1]);

    //Create socket
    socket_desc = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_desc == -1) {
        printf("Could not create socket");
    }

    int optval = 1;
    if (setsockopt(socket_desc, SOL_SOCKET, SO_REUSEADDR, (const void *) &optval, sizeof(int)) < 0) {
        perror("unable to call setsockopt");
        exit(1);
    }

    //Prepare the sockaddr_in structure
    bzero(&server, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons((uint16_t)atoi(argv[2]));

    //Bind
    if (bind(socket_desc, (struct sockaddr *) &server, sizeof(server)) < 0) {
        //print the error message
        perror("bind failed. Error");
        return 1;
    }
    puts("bind done");

    //Listen
    listen(socket_desc, LISTENQ);
    socklen = sizeof(struct sockaddr_in);
    //Accept and incoming connection
    puts("Waiting for incoming connections...");
    while (1) {
        if ((client_sock = accept(socket_desc, (struct sockaddr *) &client, &socklen)) <= 0) {
            continue;
        }
        puts("Connection accepted");

        pthread_t sniffer_thread;
        new_sock = malloc(1);
        *new_sock = client_sock;

        if (pthread_create(&sniffer_thread, NULL, connection_handler, (void *) new_sock) < 0) {
            perror("could not create thread");
            return 1;
        }

        //Now join the thread , so that we dont terminate before the thread
        pthread_join(sniffer_thread, NULL);
        puts("Handler assigned");
    }

    return 0;
}

/*
 * This will handle connection for each client
 * */
void *connection_handler(void *socket_desc)
{
    //Get the socket descriptor
    int sock = *(int *) socket_desc;
    char buffer[PACKET_SIZE];
    char msg[1024];
    uint8_t errno;
    FILE *fp = NULL;
    memset(buffer, 0, sizeof(buffer));
    while (recv(sock, buffer, sizeof(buffer), 0) > 0) {
        errno = 0;
        memset(msg, 0, sizeof(msg));
        msg_packet_t *mpkt = (msg_packet_t *) buffer;
        switch (mpkt->msg_type) {
            case CMD_LS:
                list_dir(msg, ".", '\n');
                break;
            case CMD_GET:
                if (strlen(mpkt->msg) > 0) {
                    if ((fp = fopen(mpkt->msg, "rb")) == NULL) {
                        strcpy(msg, "server: get: Cannot open the file\n");
                    }
                } else {
                    strcpy(msg, "server: get: Missing file operand\n");
                }
                break;
            case CMD_PUT:
                if (strlen(mpkt->msg) > 0) {
                    if ((fp = fopen(mpkt->msg, "wb")) == NULL) {
                        strcpy(msg, "server: put: Cannot create the file\n");
                    }
                } else {
                    strcpy(msg, "server: get: Missing file operand\n");
                }
                break;
            default:
                strcpy(msg, mpkt->msg);
        }
        build_msg_packet((msg_packet_t *) buffer, mpkt->msg_type, errno, msg, (uint16_t) strlen(msg));
        if (send(sock, buffer, PACKET_SIZE, 0) >= 0) {
            if (mpkt->msg_type == CMD_GET && fp) {
                send_file(fp, 0, -1, sock);
            } else if (mpkt->msg_type == CMD_PUT && fp) {
                receive_file(fp, sock);
            }
        } else {
            perror("fail to send.");
        }
        if (fp != NULL) {
            fclose(fp);
            fp = NULL;
        }
        memset(buffer, 0, sizeof(buffer));
    }

    printf("close...\n");
    close(sock);
    free(socket_desc);

    return 0;
}
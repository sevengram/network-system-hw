#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <unistd.h>
#include "trans.h"
#include "util.h"

#define LISTENQ         1024

server_config_t conf;

void *connection_handler(void *);

int main(int argc, char *argv[])
{
    int server_sock, client_sock, *new_sock;
    socklen_t socklen;
    struct sockaddr_in server, client;

    if (argc < 3) {
        printf("USAGE: <directory> <port> <conf>\n");
        exit(1);
    }

    if (init_server_config(&conf, argc > 3 ? argv[3] : "server.conf") == -1) {
        printf("Invalid config file.\n");
        exit(1);
    }

    if (create_dir(argv[1]) < 0) {
        perror("fail to create dir");
        exit(1);
    }

    if (chdir(argv[1]) < 0) {
        perror("fail to change the dir");
        exit(1);
    }

    //Create socket
    if ((server_sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("could not create socket");
        exit(1);
    }

    // Eliminates "Address already in use" error from bind.
    int optval = 1;
    if (setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, (const void *) &optval, sizeof(int)) < 0) {
        perror("unable to call setsockopt");
        exit(1);
    }

    //Prepare the sockaddr_in structure
    bzero(&server, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons((uint16_t) atoi(argv[2]));

    //Bind
    if (bind(server_sock, (struct sockaddr *) &server, sizeof(server)) < 0) {
        //print the error message
        perror("bind failed. Error");
        exit(1);
    }

    //Listen
    listen(server_sock, LISTENQ);
    socklen = sizeof(struct sockaddr_in);
    //Accept and incoming connection
    printf("Waiting for incoming connections...\n");
    while (1) {
        if ((client_sock = accept(server_sock, (struct sockaddr *) &client, &socklen)) <= 0) {
            continue;
        }
        printf("Connection accepted\n");

        pthread_t t;
        new_sock = malloc(1);
        *new_sock = client_sock;

        if (pthread_create(&t, NULL, connection_handler, new_sock) < 0) {
            perror("could not create thread");
            return 1;
        }
    }

    return 0;
}

int check_user(const char *username, const char *password)
{
    int i;
    for (i = 0; i < conf.nu; i++) {
        if (strcmp(username, conf.users[i].username) == 0) {
            if (strcmp(password, conf.users[i].password) == 0) {
                return 1;
            } else {
                return -1;
            }
        }
    }
    return -1;
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
    char path[256];
    uint8_t errno;
    FILE *fp = NULL;
    bzero(buffer, sizeof(buffer));
    while (recv(sock, buffer, sizeof(buffer), 0) > 0) {
        errno = 0;
        bzero(msg, sizeof(msg));
        msg_packet_t *mpkt = (msg_packet_t *) buffer;
        if (check_user(mpkt->header.username, mpkt->header.password) <= 0) {
            strcpy(msg, "Invalid Username/Password. Please try again.");
            errno = 1;
        } else {
            if (create_dir(mpkt->header.username) < 0) {
                strcpy(msg, "Fail to create user's dir. Please try again.");
                perror("fail to create dir");
                errno = 1;
            } else {
                bzero(path, sizeof(path));
                switch (mpkt->msg_type) {
                    case CMD_MKDIR:
                        set_path(path, mpkt->header.username, mpkt->msg);
                        if (create_dir(path) < 0) {
                            strcpy(msg, "Fail to mkdir");
                            perror("fail to create dir");
                            errno = 1;
                        }
                        break;
                    case CMD_LS:
                        set_path(path, mpkt->header.username, mpkt->msg);
                        list_dir(msg, path, '\n');
                        break;
                    case CMD_GET:
                        if (strlen(mpkt->msg) > 0) {
                            set_path(path, mpkt->header.username, mpkt->msg);
                            if ((fp = fopen(path, "rb")) == NULL) {
                                strcpy(msg, "server: get: cannot open the file");
                                errno = 1;
                            }
                        } else {
                            strcpy(msg, "server: get: missing file operand");
                            errno = 1;
                        }
                        break;
                    case CMD_PUT:
                        if (strlen(mpkt->msg) > 0) {
                            set_path(path, mpkt->header.username, mpkt->msg);
                            if ((fp = fopen(path, "wb")) == NULL) {
                                strcpy(msg, "server: put: Cannot create the file");
                                errno = 1;
                            }
                        } else {
                            strcpy(msg, "server: get: Missing file operand");
                            errno = 1;
                        }
                        break;
                    default:
                        strcpy(msg, mpkt->msg);
                }
            }
        }
        build_msg_packet((msg_packet_t *) buffer, mpkt->msg_type, errno, msg, (uint16_t) strlen(msg), 0);
        if (send(sock, buffer, PACKET_SIZE, 0) >= 0) {
            if (mpkt->msg_type == CMD_GET && fp) {
                send_file(fp, 0, -1, sock, 0);
            } else if (mpkt->msg_type == CMD_PUT && fp) {
                receive_file(fp, sock, 0);
            }
        } else {
            perror("fail to send.");
        }
        if (fp != 0) {
            fclose(fp);
            fp = 0;
        }
        memset(buffer, 0, sizeof(buffer));
    }

    printf("close...\n");
    close(sock);
    free(socket_desc);

    return 0;
}
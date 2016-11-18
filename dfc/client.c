#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <pthread.h>
#include "transmission.h"
#include "strutil.h"
#include "util.h"

#define BUF_SIZE 4096
#define MAX_CONN 32
#define MAX_NAME 256

client_config_t conf;

void *connection_handler(void *p);

typedef struct
{
    int sock;
    int parts[2];
    char path[MAX_NAME];
    char filename[MAX_NAME];
    msg_packet_t *pkt;
} thread_param;

int main(int argc, char *argv[])
{
    int socks[MAX_CONN];
    pthread_t threads[MAX_CONN];
    void *retval[MAX_CONN];

    if (init_client_config(&conf, argc > 1 ? argv[1] : "client.conf") == -1) {
        printf("Invalid config file.\n");
        exit(1);
    }

    int i, j, k, pi, n;
    for (i = 0; i < conf.nh; i++) {
        if ((socks[i] = open_clientfd(conf.hosts[i].ip, conf.hosts[i].port)) < 0) {
            conf.hosts[i].available = 0;
            perror("unable to connect server");
        } else {
            conf.hosts[i].available = 1;
        }
    }

    int retry;
    uint8_t cmd_type;
    char *msg = 0;
    char buffer[BUF_SIZE];
    int count[BUF_SIZE];
    char **p = malloc_strings(BUF_SIZE, 256);
    msg_packet_t *pkt = calloc(1, sizeof(msg_packet_t));
    int hashvalue;
    int flag = 0;
    while (1) {
        printf("> ");
        retry = 1;
        hashvalue = 0;
        bzero(buffer, BUF_SIZE);
        clear_strings(p, BUF_SIZE, 256);
        if (fgets(buffer, BUF_SIZE, stdin) != NULL) {
            remove_endl(buffer);
            strip(buffer, ' ');
            n = split(buffer, " ", p);
            if (n == 0) {
                continue;
            }
            lower(p[0]);
            if (n > 0 && strcmp(p[0], "mkdir") == 0) {
                cmd_type = CMD_MKDIR;
                if (n > 1) {
                    msg = p[1];
                } else {
                    printf("mkdir: Missing file operand\n");
                    continue;
                }
            } else if (n > 0 && strcmp(p[0], "list") == 0) {
                cmd_type = CMD_LS;
                if (n > 1) {
                    msg = p[1];
                }
            } else if (n > 0 && strcmp(p[0], "get") == 0) {
                cmd_type = CMD_GET;
                if (n <= 1) {
                    printf("get: Missing file operand\n");
                    continue;
                }
            } else if (n > 0 && strcmp(p[0], "put") == 0) {
                cmd_type = CMD_PUT;
                if (n <= 1) {
                    printf("put: Missing file operand\n");
                    continue;
                }
            } else {
                continue;
            }

            build_msg_packet(pkt, cmd_type, 0, msg, (uint16_t) ((msg == NULL) ? 0 : strlen(msg)), &conf);

            while (retry) {
                flag = !conf.hosts[conf.nh - 1].available;
                for (i = 0; i < conf.nh; i++) {
                    if (conf.hosts[i].available) {
                        thread_param *tp = calloc(1, sizeof(thread_param));
                        tp->sock = socks[i];
                        tp->pkt = pkt;
                        if (cmd_type == CMD_PUT || cmd_type == CMD_GET) {
                            if (n > 1) {
                                strcpy(tp->filename, p[1]);
                                hashvalue = md5_mod(tp->filename, conf.nh);
                            }
                            if (n > 2) {
                                strcpy(tp->path, p[2]);
                            }
                        }
                        if (cmd_type == CMD_PUT || (cmd_type == CMD_GET && flag)) {
                            tp->parts[0] = (i - hashvalue + conf.nh) % conf.nh;
                            tp->parts[1] = (i - hashvalue + 1 + conf.nh) % conf.nh;
                        } else {
                            tp->parts[0] = -1;
                            tp->parts[1] = (i - hashvalue + 1 + conf.nh) % conf.nh;
                        }
                        if (pthread_create(threads + i, NULL, connection_handler, tp) < 0) {
                            perror("could not create thread");
                        }
                        flag = 0;
                    } else {
                        flag = 1;
                    }
                }

                bzero(retval, 32 * sizeof(void *));
                for (i = 0; i < conf.nh; i++) {
                    if (conf.hosts[i].available) {
                        if (pthread_join(threads[i], retval + i) < 0) {
                            perror("could not join thread");
                        }
                    }
                }

                flag = 0;
                retry = 0;
                if (cmd_type == CMD_GET) {
                    for (i = 0; i < conf.nh; i++) {
                        if ((long) retval[i] == 1) {
                            conf.hosts[i].available = 0;
                            retry = 1;
                        } else if ((long) retval[i] == 2) {
                            flag = 1;
                        }
                    }
                    if (!retry && !flag) {
                        merge_partitions(p[1], conf.nh);
                        delete_partitions(p[1], conf.nh);
                    }
                } else if (cmd_type == CMD_LS) {
                    bzero(buffer, BUF_SIZE);
                    bzero(count, BUF_SIZE * sizeof(int));
                    for (i = 0; i < conf.nh; i++) {
                        if ((long) retval[i] == 1) {
                            conf.hosts[i].available = 0;
                        } else if (retval[i] != 0) {
                            strcat(buffer, retval[i]);
                            free(retval[i]);
                        }
                    }
                    j = 0;
                    n = split(buffer, "\n", p);
                    for (i = 0; i < n; i++) {
                        flag = 0;
                        bzero(buffer, BUF_SIZE);
                        pi = parse_partition_name(buffer, p[i]);
                        for (k = 0; k < j; k++) {
                            if (strcmp(p[k], buffer) == 0) {
                                count[k] |= (1 << pi);
                                flag = 1;
                            }
                        }
                        if (!flag) {
                            strcpy(p[j], buffer);
                            count[j] |= (1 << pi);
                            j++;
                        }
                    }
                    for (i = 0; i < j; i++) {
                        if (count[i] == 15) {
                            printf("%s\n", p[i]);
                        } else {
                            printf("%s [imcomplete]\n", p[i]);
                        }
                    }
                }
            }
        }
    }
    free_strings(p, 4096);
    free(pkt);
    for (i = 0; i < conf.nh; i++) {
        if (socks[i] > 0) {
            close(socks[i]);
        }
    }
    return 0;
}

void *handle_put(thread_param *tp)
{
    int i;
    long fsize, psize;
    FILE *fp;
    msg_packet_t *mpkt, *outpkt;
    char buffer[PACKET_SIZE];
    long retval = 0;
    if ((fp = fopen(tp->filename, "rb")) == 0) {
        perror("cannot open the file");
        return 0;
    }

    outpkt = calloc(1, sizeof(msg_packet_t));
    bcopy(tp->pkt, outpkt, sizeof(msg_packet_t));

    fsize = size_of_fp(fp);
    psize = fsize / conf.nh + 1;
    for (i = 0; i < 2; i++) {
        bzero(buffer, PACKET_SIZE);
        set_partition_name(buffer, tp->filename, tp->parts[i]);
        set_path(outpkt->msg, tp->path, buffer);

        if (send(tp->sock, outpkt, PACKET_SIZE, 0) < 0) {
            //perror("fail to send");
            retval = 1;
        }
        bzero(buffer, PACKET_SIZE);
        if (recv(tp->sock, buffer, PACKET_SIZE, 0) < 0) {
            //perror("fail to recv");
            retval = 1;
        }
        mpkt = (msg_packet_t *) buffer;
        if (strlen(mpkt->msg) != 0) {
            printf("%s\n", mpkt->msg);
        }
        if (mpkt->errno == 0) {
            send_file(fp, psize * tp->parts[i], psize, tp->sock, conf.password);
        }
    }
    fclose(fp);
    free(outpkt);
    return (void *) retval;
}

void *handle_get(thread_param *tp)
{
    int i;
    FILE *fp;
    msg_packet_t *mpkt, *outpkt;
    char filename[MAX_NAME];
    char buffer[PACKET_SIZE];

    outpkt = calloc(1, sizeof(msg_packet_t));
    bcopy(tp->pkt, outpkt, sizeof(msg_packet_t));
    long retval = 0;
    for (i = 0; i < 2; i++) {
        if (tp->parts[i] != -1) {
            bzero(filename, sizeof(filename));
            set_partition_name(filename, tp->filename, tp->parts[i]);
            set_path(outpkt->msg, tp->path, filename);
            if (send(tp->sock, outpkt, PACKET_SIZE, 0) < 0) {
                //perror("fail to send");
                retval = 1;
                break;
            }
            if (recv(tp->sock, buffer, PACKET_SIZE, 0) < 0) {
                //perror("fail to recv");
                retval = 1;
                break;
            }
            mpkt = (msg_packet_t *) buffer;
            if (strlen(mpkt->msg) != 0) {
                printf("%s\n", mpkt->msg);
            }
            if (mpkt->errno == 0) {
                if ((fp = fopen(filename, "wb")) == 0) {
                    perror("cannot create the file");
                    retval = 0;
                    break;
                }
                if (receive_file(fp, tp->sock, conf.password) < 0) {
                    retval = 1;
                    break;
                }
                fclose(fp);
            } else {
                retval = 2;
                break;
            }
        }
    }
    free(outpkt);
    return (void *) retval;
}

void *handle_ls(thread_param *tp)
{
    char buffer[PACKET_SIZE];
    char *result = calloc(sizeof(char), PACKET_SIZE);

    if (send(tp->sock, tp->pkt, PACKET_SIZE, 0) < 0) {
        //perror("fail to send");
        return (void *) 1;
    }
    if (recv(tp->sock, buffer, PACKET_SIZE, 0) < 0) {
        //perror("fail to recv");
        return (void *) 1;
    }
    msg_packet_t *mpkt = (msg_packet_t *) buffer;
    if (strlen(mpkt->msg) != 0) {
        if (mpkt->errno == 1) {
            printf("%s\n", mpkt->msg);
        } else {
            strcpy(result, mpkt->msg);
        }
    }
    return result;
}

void *handle_mkdir(thread_param *tp)
{
    char buffer[PACKET_SIZE];
    if (send(tp->sock, tp->pkt, PACKET_SIZE, 0) < 0) {
        //perror("fail to send");
        return (void *) 1;
    }
    if (recv(tp->sock, buffer, PACKET_SIZE, 0) < 0) {
        //perror("fail to recv");
        return (void *) 1;
    }
    msg_packet_t *mpkt = (msg_packet_t *) buffer;
    if (strlen(mpkt->msg) != 0) {
        printf("%s\n", mpkt->msg);
    }
    return 0;
}

void *connection_handler(void *p)
{
    thread_param *tp = p;
    void *result = 0;
    if (tp->pkt->msg_type == CMD_LS) {
        result = handle_ls(tp);
    } else if (tp->pkt->msg_type == CMD_PUT) {
        result = handle_put(tp);
    } else if (tp->pkt->msg_type == CMD_GET) {
        result = handle_get(tp);
    } else if (tp->pkt->msg_type == CMD_MKDIR) {
        result = handle_mkdir(tp);
    }
    free(p);
    return result;
}
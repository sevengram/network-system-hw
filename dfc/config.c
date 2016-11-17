#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "config.h"
#include "strutil.h"
#include "util.h"

int init_client_config(client_config_t *conf, const char *conf_file)
{
    FILE *fp = fopen(conf_file, "r");
    if (fp == 0) {
        return -1;
    }
    int n;
    char ip[16];
    char port[8];
    char buf[128];
    char **p = malloc_strings(16, 128);

    bzero(buf, sizeof(buf));
    bzero(conf, sizeof(client_config_t));

    while (fgets(buf, sizeof(buf), fp) != 0) {
        remove_endl(buf);
        strip(buf, ' ');
        if (strlen(buf) > 0 && buf[0] != '#') {
            n = split(buf, " \t", p);
            if (n > 1) {
                strip(p[0], ':');
                lower(p[0]);
                if (strcmp(p[0], "username") == 0) {
                    strcpy(conf->username, p[1]);
                } else if (strcmp(p[0], "password") == 0) {
                    strcpy(conf->password, p[1]);
                } else if (strcmp(p[0], "server") == 0) {
                    if (n >= 3) {
                        split_first(p[2], ':', ip, port);
                        conf->hosts[conf->nh].port = (uint16_t) atoi(port);
                        strcpy(conf->hosts[conf->nh++].ip, ip);
                    }
                }
            }
        }
    }
    free_strings(p, 16);
    fclose(fp);
    return 0;
}

int init_server_config(server_config_t *conf, const char *conf_file)
{
    FILE *fp = fopen(conf_file, "r");
    if (fp == 0) {
        return -1;
    }
    int n;
    char buf[128];
    char **p = malloc_strings(16, 128);
    bzero(conf, sizeof(server_config_t));
    while (fgets(buf, sizeof(buf), fp) != 0) {
        remove_endl(buf);
        strip(buf, ' ');
        if (strlen(buf) > 0 && buf[0] != '#') {
            n = split(buf, " \t", p);
            if (n > 1) {
                strcpy(conf->users[conf->nu].username, p[0]);
                strcpy(conf->users[conf->nu].password, p[1]);
                conf->nu++;
            }
        }
    }
    return 0;
}

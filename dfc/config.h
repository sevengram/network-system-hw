#ifndef NETWORKS_CONFIG_H
#define NETWORKS_CONFIG_H

#include <stdint.h>

typedef struct
{
    char ip[16];
    uint16_t port;
    uint8_t available;
} host_t;

typedef struct
{
    int nh; // number of hosts
    host_t hosts[32];
    char username[16];
    char password[16];
} client_config_t;

typedef struct
{
    char username[16];
    char password[16];
} user_t;

typedef struct
{
    int nu; // number of users
    user_t users[32];
} server_config_t;


int init_client_config(client_config_t *conf, const char *conf_file);

int init_server_config(server_config_t *conf, const char *conf_file);

#endif //NETWORKS_CONFIG_H

#ifndef NETWORKS_CONFIG_H
#define NETWORKS_CONFIG_H

#include <stdint.h>

typedef struct
{
    uint16_t port;
    uint32_t keepalive_time;
    uint16_t count_content_types;
    char root[256];
    char index[32][128];
    char file_extensions[32][128];
    char content_types[32][128];
} config_t;

int init_config(config_t *conf, const char *conf_file);

void get_content_type(config_t *conf, const char *file_extension, char *content_type);

#endif //NETWORKS_CONFIG_H

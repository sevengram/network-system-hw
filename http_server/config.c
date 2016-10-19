#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "config.h"
#include "strutil.h"
#include "util.h"

int init_config(config_t *conf, const char *conf_file)
{
    // TODO: error check
    FILE *fp = fopen(conf_file, "r");
    if (fp == 0) {
        return -1;
    }
    int i, n;
    uint16_t k = 0;
    char buf[128];
    char **p = malloc_strings(32, 128);

    bzero(buf, sizeof(buf));
    bzero(conf, sizeof(config_t));

    // Some default settings
    conf->port = 80;
    strcpy(conf->root, ".");
    strcpy(conf->index[0], "index.html");
    strcpy(conf->file_extensions[0], ".html");
    strcpy(conf->content_types[0], "text/html");

    while (fgets(buf, sizeof(buf), fp) != 0) {
        remove_endl(buf);
        strip(buf, ' ');
        if (strlen(buf) > 0 && buf[0] != '#') {
            n = split(buf, " \t", p);
            if (n > 1) {
                lower(p[0]);
                if (strcmp(p[0], "listen") == 0) {
                    for (i = 1; i < n; i++) {
                        if (strlen(p[i]) != 0) {
                            conf->port = (uint16_t) atoi(p[i]);
                            break;
                        }
                    }
                } else if (strcmp(p[0], "documentroot") == 0) {
                    for (i = 1; i < n; i++) {
                        if (strlen(p[i]) != 0) {
                            strip(p[i], '\"');
                            strcpy(conf->root, p[i]);
                            break;
                        }
                    }
                } else if (strcmp(p[0], "directoryindex") == 0) {
                    int j = 0;
                    for (i = 1; i < n; i++) {
                        if (strlen(p[i]) != 0) {
                            strcpy(conf->index[j++], p[i]);
                        }
                    }
                } else if (strcmp(p[0], "keepalivetime") == 0) {
                    for (i = 1; i < n; i++) {
                        if (strlen(p[i]) != 0) {
                            conf->keepalive_time = (uint32_t) atoi(p[i]);
                            break;
                        }
                    }
                } else if (strcmp(p[0], "contenttype") == 0) {
                    for (i = 1; i < n; i++) {
                        if (strlen(p[i]) != 0) {
                            strcpy(conf->file_extensions[k], p[i]);
                            break;
                        }
                    }
                    for (i = i + 1; i < n; i++) {
                        if (strlen(p[i]) != 0) {
                            strcpy(conf->content_types[k++], p[i]);
                            break;
                        }
                    }
                }
            }
        }
    }
    conf->count_content_types = k;
    free_strings(p, 32);
    fclose(fp);
    return 0;
}

void get_content_type(config_t *conf, const char *filename, char *content_type)
{
    char extension[64];
    bzero(extension, sizeof(extension));
    get_file_extension(filename, extension);
    if (strlen(extension) != 0) {
        int i;
        for (i = 0; i < conf->count_content_types; i++) {
            if (strcmp(conf->file_extensions[i], extension) == 0) {
                strcpy(content_type, conf->content_types[i]);
                return;
            }
        }
    }
    strcpy(content_type, "");
}
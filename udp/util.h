#ifndef NETWORKS_UTIL_H
#define NETWORKS_UTIL_H

int parse_line(char *cmdline, char **argv);

void remove_endl(char *line);

int is_endl(char *line);

void list_dir(char *buffer, const char *name, char delimiter);

#endif //NETWORKS_UTIL_H

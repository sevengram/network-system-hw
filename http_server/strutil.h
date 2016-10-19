#ifndef NETWORKS_STRUTIL_H
#define NETWORKS_STRUTIL_H

int split(char *str, char *sep, char** results);

void remove_endl(char *str);

void strip(char *str, char sep);

void lower(char *str);

void split_first(char *str, char sep, char *first, char *second);

#endif //NETWORKS_STRUTIL_H

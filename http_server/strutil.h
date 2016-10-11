//
// Created by jfan on 10/11/16.
//

#ifndef NETWORKS_STRUTIL_H
#define NETWORKS_STRUTIL_H

#define LINE_LEN_MAX 1024

int split(char *str, char *sep, char results[][LINE_LEN_MAX]);

void remove_endl(char *line);

void strip(char *line);

#endif //NETWORKS_STRUTIL_H

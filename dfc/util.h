#ifndef HTTP_SERVER_UTIL_H
#define HTTP_SERVER_UTIL_H

#include "strutil.h"

/* type of message */
#define CMD_LS       1
#define CMD_PUT      2
#define CMD_GET      3
#define CMD_MKDIR    4

#define MAX(x, y) (((x)>(y))?(x):(y))
#define MIN(x, y) (((x)<(y))?(x):(y))


void format_current_time(char *buf, size_t maxsize);

long size_of_file(const char *filename);

long size_of_fp(FILE *fp);

char **malloc_strings(size_t n, size_t len);

void free_strings(char **c, size_t n);

void get_file_extension(const char *filename, char *extension);

void set_partition_name(char *partname, const char *filename, int n);

int parse_partition_name(char *filename, const char *partname);

int merge_partitions(const char *filename, int n);

int delete_partitions(const char *filename, int n);

int open_clientfd(char *hostname, uint16_t port);

int create_dir(const char *dir);

void set_path(char *path, const char *p1, const char *p2);

int md5_mod(const char *filename, int n);

#endif //HTTP_SERVER_UTIL_H

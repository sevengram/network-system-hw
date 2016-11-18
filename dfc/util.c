#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/socket.h>
#include <netdb.h>
#include <openssl/md5.h>
#include "util.h"

void format_current_time(char *buf, size_t maxsize)
{
    time_t now = time(0);
    struct tm gt = *gmtime(&now);
    strftime(buf, maxsize, "%a, %d %b %Y %H:%M:%S %Z", &gt);
}

long size_of_file(const char *filename)
{
    struct stat st;
    if (stat(filename, &st) == -1) {
        return -1;
    } else {
        return st.st_size;
    }
}

long size_of_fp(FILE *fp)
{
    fseek(fp, 0, SEEK_END);
    long len = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    return len;
}

char **malloc_strings(size_t n, size_t len)
{
    int i;
    char **c = calloc(n, sizeof(char *));
    for (i = 0; i < n; i++) {
        c[i] = calloc(len, sizeof(char));
    }
    return c;
}

void clear_strings(char **c, size_t n, size_t len)
{
    int i;
    for (i = 0; i < n; i++) {
        bzero(c[i], len);
    }
}

void free_strings(char **c, size_t n)
{
    int i;
    for (i = 0; i < n; i++) {
        free(c[i]);
    }
    free(c);
}

void get_file_extension(const char *filename, char *extension)
{
    char *p = strrchr(filename, '.');
    if (p != 0) {
        strcpy(extension, p);
    } else {
        strcpy(extension, "");
    }
}

void set_partition_name(char *partname, const char *filename, int n)
{
    sprintf(partname, ".%s.%d", filename, n);
}

int parse_partition_name(char *filename, const char *partname)
{
    char *c = strrchr(partname, '.');
    strncpy(filename, partname + 1, c - partname - 1);
    return atoi(c + 1);
}

int merge_partitions(const char *filename, int n)
{
    FILE *fp_read, *fp_write;
    if ((fp_write = fopen(filename, "wb")) == 0) {
        perror("can't create file");
        return -1;
    }

    int i;
    char buffer[4096];
    char pname[128];
    size_t nbytes;

    for (i = 0; i < n; ++i) {
        set_partition_name(pname, filename, i);
        if ((fp_read = fopen(pname, "rb")) == 0) {
            printf("incomplete file\n");
            break;
        }

        while ((nbytes = fread(buffer, sizeof(char), 4096, fp_read)) > 0) {
            if (fwrite(buffer, sizeof(char), nbytes, fp_write) < 0) {
                perror("can't write files");
                break;
            }
        }
        fclose(fp_read);
    }
    fclose(fp_write);
    return 0;
}

int delete_partitions(const char *filename, int n)
{
    int i;
    char pname[128];
    for (i = 0; i < n; ++i) {
        set_partition_name(pname, filename, i);
        unlink(pname);
    }
    return 0;
}


/*
 * open_clientfd - open connection to server at <hostname, port>
 *   and return a socket descriptor ready for reading and writing.
 *   Returns -1 and sets errno on Unix error.
 *   Returns -2 and sets h_errno on DNS (gethostbyname) error.
 */
int open_clientfd(char *hostname, uint16_t port)
{
    int clientfd;
    struct hostent *hp;
    struct sockaddr_in serveraddr;

    if ((clientfd = socket(PF_INET, SOCK_STREAM, 0)) < 0)
        return -1; /* check errno for cause of error */

    /* Fill in the server's IP address and port */
    if ((hp = gethostbyname(hostname)) == NULL)
        return -2; /* check h_errno for cause of error */

    bzero((char *) &serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_port = htons(port);
    bcopy(hp->h_addr_list[0], (char *) &serveraddr.sin_addr.s_addr, hp->h_length);

    /* Establish a connection with the server */
    if (connect(clientfd, (struct sockaddr *) &serveraddr, sizeof(serveraddr)) < 0)
        return -1;
    return clientfd;
}

int create_dir(const char *dir)
{
    struct stat s;
    int err = stat(dir, &s);
    if (-1 == err) {
        if (ENOENT == errno) {
            if (mkdir(dir, 0755) == 0) {
                return 1;
            } else {
                return -1;
            }
        } else {
            return -1;
        }
    } else {
        if (S_ISDIR(s.st_mode)) {
            return 0;
        } else {
            return -1;
        }
    }
}

void set_path(char *path, const char *p1, const char *p2)
{
    sprintf(path, "./%s/%s", p1, p2);
}

int md5_mod(const char *filename, int n)
{
    unsigned char c[MD5_DIGEST_LENGTH];
    MD5_CTX md_context;
    MD5_Init(&md_context);
    MD5_Update(&md_context, filename, strlen(filename));
    MD5_Final(c, &md_context);
    return c[0] % n;
}
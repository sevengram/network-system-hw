#include <sys/socket.h>
#include <time.h>
#include <stdio.h>
#include <netinet/in.h>

void format_log_entry(char *logstring, int sock, char *uri, int size)
{
    time_t now;
    char buffer[1024];
    struct sockaddr_in addr;
    unsigned long host;
    unsigned long a, b, c, d;
    int len = sizeof(addr);

    now = time(NULL);
    strftime(buffer, 1024, "%a %d %b %Y %H:%M:%S %Z", localtime(&now));

    if (getpeername(sock, (struct sockaddr *) &addr, (socklen_t *) &len)) {
        return;
    }

    host = ntohl(addr.sin_addr.s_addr);
    a = host >> 24;
    b = (host >> 16) & 0xff;
    c = (host >> 8) & 0xff;
    d = host & 0xff;

    sprintf(logstring, "%s: %ld.%ld.%ld.%ld %s %d", buffer, a, b, c, d, uri, size);
}

void log_info(int fd, char *uri, int size)
{
    char logString[1024];
    format_log_entry(logString, fd, uri, size);
    printf("%s\n", logString);
}

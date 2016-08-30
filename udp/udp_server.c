#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>

/* You will have to modify the program below */

#define MAXBUFSIZE 100

int main(int argc, char *argv[])
{
    int sock;                           //This will be our socket
    struct sockaddr_in sin, remote;     //"Internet socket address structure"
    socklen_t addr_length;              //length of the sockaddr_in structure
    char buffer[MAXBUFSIZE];            //a buffer to store our received message
    ssize_t nbytes;                     //number of bytes we receive in our message

    if (argc != 2) {
        printf("USAGE:  <port>\n");
        exit(1);
    }

    addr_length = sizeof(struct sockaddr);

    /*
     * This code populates the sockaddr_in struct with
     * the information about our socket
     */
    bzero(&sin, sizeof(sin));                   //zero the struct
    sin.sin_family = AF_INET;                   //address family
    sin.sin_port = htons(atoi(argv[1]));        //htons() sets the port # to network byte order
    sin.sin_addr.s_addr = INADDR_ANY;           //supplies the IP address of the local machine

    // Causes the system to create a generic socket of type UDP (datagram)
    if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        printf("unable to create socket");
        exit(1);
    }

    // Eliminates "Address already in use" error from bind.
    int optval = 1;
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (const void *) &optval, sizeof(int)) < 0) {
        printf("unable to call setsockopt");
        exit(1);
    }

    /*
     * Once we've created a socket, we must bind that socket to the
     * local address and port we've supplied in the sockaddr_in struct
     */
    if (bind(sock, (struct sockaddr *) &sin, sizeof(sin)) < 0) {
        printf("unable to bind socket\n");
    }

    // waits for an incoming message
    bzero(buffer, sizeof(buffer));
    // TODO:
    nbytes = recvfrom(sock, buffer, sizeof(buffer), 0, (struct sockaddr *) &remote, &addr_length);

    printf("The client says %s\n", buffer);

    char msg[] = "orange";
    // TODO:
    nbytes = sendto(sock, msg, strlen(msg), 0, (struct sockaddr *) &remote, addr_length);

    close(sock);
    return 0;
}


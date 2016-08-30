#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>

#define MAXBUFSIZE 100

/* You will have to modify the program below */

int main(int argc, char *argv[])
{
    int sock;                               //this will be our socket
    struct sockaddr_in remote;              //"Internet socket address structure"
    socklen_t addr_length;                  //length of the sockaddr_in structure
    char buffer[MAXBUFSIZE];
    ssize_t nbytes;                         // number of bytes send by sendto()

    if (argc < 3) {
        printf("USAGE:  <server_ip> <server_port>\n");
        exit(1);
    }

    addr_length = sizeof(struct sockaddr);

    /*
     * Here we populate a sockaddr_in struct with
     * information regarding where we'd like to send our packet
     * i.e the Server.
     */
    bzero(&remote, sizeof(remote));              //zero the struct
    remote.sin_family = AF_INET;                 //address family
    remote.sin_port = htons(atoi(argv[2]));      //sets port to network byte order
    remote.sin_addr.s_addr = inet_addr(argv[1]); //sets remote IP address

    //Causes the system to create a generic socket of type UDP (datagram)
    if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        printf("unable to create socket");
        exit(1);
    }

    /*
     * sendto() sends immediately.
     * it will report an error if the message fails to leave the computer
     * however, with UDP, there is no error if the message is lost in the network
     * once it leaves the computer.
     */
    char command[] = "apple";
    // TODO:
    nbytes = sendto(sock, command, strlen(command), 0, (struct sockaddr *) &remote, addr_length);

    bzero(buffer, sizeof(buffer));
    // TODO:
    nbytes = recvfrom(sock, buffer, sizeof(buffer), 0, (struct sockaddr *) &remote, &addr_length);

    printf("Server says %s\n", buffer);

    close(sock);
    return 0;
}


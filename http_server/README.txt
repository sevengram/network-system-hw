Programming Assignment 2

Jianxiang Fan
jianxiang.fan@colorado.edu


>>> How to run the program

$ make clean
$ make all
$ ./webServer


>>> Configuration file

User can specify the configuraion file by adding a parameter to the command line:
./webServer test.conf

By default, the program will run with ws.conf.

The file is used to modify all configuration parameters, some default values are used:

Listen (Default: 80)
DocumentRoot (Default: current working directory)
DirectoryIndex (Default: index.html)
ContentType (Default: .html text/index.html)

If the configuration file is not found, the server won't start.
Port numbers below 1024 will be rejected.


>>> Multiple Connections

I use I/O multiplexing (epoll) with non-blocking I/O to support multiple connections.
Epoll is a scalable I/O event notification mechanism in Linux. Its function is to monitor
multiple file descriptors to see if I/O is possible on any of them. It has a better
performance than older POSIX select and poll system calls (select and poll operate in
O(n) time, epoll operates in O(1) time)


>>> POST method
POST method is supported. POST data is appended at the end of webpage.


>>> Error Handling
400: The request message was in a wrong format
404: Requested URL doesn't exist
501: Requested file type or method or http version is not supported
500: Internal Server Error
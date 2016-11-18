Programming Assignment 3

Jianxiang Fan
jianxiang.fan@colorado.edu


>>> How to run the programs

$ make
$ ./run_servers.sh
$ ./client


>>> Configuration file

For client: client.conf

For server: server.conf

>>> Data encryption

Encrypt the data by applying XOR operation with the user's password


>>> Traffic Optimization

When all servers are active, the client gets one piece from each server to reconstruct the original file.

If server i is down, the client will get server i's unavailable pieces from server i-1 and server i+1 to ensure
the integrity


>>> Subfolder

Implemented
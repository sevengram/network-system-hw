#!/bin/bash

make

pkill -f dfc

./dfc ./DFS1 10001 &
./dfc ./DFS2 10002 &
./dfc ./DFS3 10003 &
./dfc ./DFS4 10004 &

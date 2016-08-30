cmake_minimum_required(VERSION 3.2)
project(networks)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu11")

add_executable(udp_client udp/udp_client.c)

add_executable(udp_server udp/udp_server.c)
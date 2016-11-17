cmake_minimum_required(VERSION 3.2)
project(networks)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu11 -pthread")

# add_executable(udp_client udp/client.c udp/util.c)

# add_executable(udp_server udp/server.c udp/util.c)

# add_executable(http_server http_server/server.c http_server/util.c
#       http_server/strutil.c http_server/http.c
#       http_server/config.c)

link_libraries(ssl crypto)

add_executable(dfc dfc/server.c dfc/util.c dfc/strutil.c dfc/transmission.c dfc/config.c)

add_executable(client dfc/client.c dfc/util.c dfc/strutil.c dfc/transmission.c dfc/config.c)

#ifndef C_AUX_HANDLERS_H
#define C_AUX_HANDLERS_H

#define _GNU_SOURCE         
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

struct client_info {
    char ip[16];
    uint16_t port;
    uint16_t P2P_port;
    bool main;
};


/*    LISTENER PROCESS                                                        */
pid_t listener_process(int client_sockfd, bool is_udp,
                        struct sockaddr_in * serv_addr,
                        int listener_pipe);

/*    SENDER PROCESS                                                          */
pid_t sender_process(int client_sockfd, bool is_udp,
                        struct sockaddr_in * serv_addr,
                        int sender_pipe);

/*    FRONT END PROCESS                                                       */
pid_t front_end_process(int back_end_pipe, int front_end_pipe);

#endif
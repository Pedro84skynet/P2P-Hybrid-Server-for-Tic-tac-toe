#ifndef CLIENT_HANDLER_H
#define CLIENT_HANDLER_H

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

/*****************************************************************************************************/
/*    CLIENT HANDLER                                                                                 */
/*****************************************************************************************************/
void client_handler(bool is_udp, int pipe_read, int pipe_write, uint16_t port, int tcp_fd);

/*****************************************************************************************************/
/*    TCP CLIENT HANDLER                                                                             */
/*****************************************************************************************************/
void tcp_client_handler(int pipe_read, int pipe_write, int tcp_fd, struct sockaddr_in * addr);


#endif
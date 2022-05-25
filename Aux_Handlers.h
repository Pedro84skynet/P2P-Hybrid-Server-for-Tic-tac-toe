/******************************************************************************
 *  Compilation:  (Use make)
 *  Execution:    ---
 *
 *
 *  DESCRIPTION
 *
 *  Auxiliars Functions for the server to handle requests, divided in two:
 *
 *    - Master Handler: used to modify log.txt and communicate with master
 *      proccess;
 *
 *    - Client Handler: used by the server to communicate with client.
 *
 *  PROJECT DECISIONS OR UNFINISHED TASKS (?)
 *
 *  List them bellow
 *
 *  - 
 *
 ******************************************************************************/

#ifndef AUX_HANDLERS_H
#define AUX_HANDLERS_H

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

/******************************************************************************/
/*    MASTER HANDLER                                                          */
/******************************************************************************/
void master_handler(int player_rd, char * client_message);

/******************************************************************************/
/*    CLIENT HANDLER                                                          */
/******************************************************************************/
int client_handler(bool is_udp, int pipe_read, int pipe_write, 
                    uint16_t port, int tcp_fd);


#endif

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

#ifndef C_AUX_HANDLERS_H
#define C_AUX_HANDLERS_H

#define _GNU_SOURCE         
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>


/*    LISTENER PROCESS                                                         */
pid_t listener_process(int client_sockfd, bool is_udp,
                        struct sockaddr_in * serv_addr,
                        int listener_pipe);

/*    SENDER PROCESS                                                           */
pid_t sender_process(int client_sockfd, bool is_udp,
                        struct sockaddr_in * serv_addr,
                        int sender_pipe);

/*    FRONT END PROCESS                                                        */
pid_t front_end_process(int back_end_pipe, int front_end_pipe);

/*    CONNECT PROCEDURE                                                        */
int Connect_Procedure(bool is_udp, int client_sockfd, struct sockaddr_in * serv_addr);

#endif
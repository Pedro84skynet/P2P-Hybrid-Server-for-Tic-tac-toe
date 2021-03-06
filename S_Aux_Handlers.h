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

#ifndef S_AUX_HANDLERS_H
#define S_AUX_HANDLERS_H

#define _GNU_SOURCE         
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

/*signal to log close*/
void handler_close(int sig);

/******************************************************************************/
/*    MASTER HANDLER                                                          */
/******************************************************************************/
int master_handler(int player_rd[128][2], char * client_message, 
                   bool DEBUG, int this_pipe);

/******************************************************************************/
/*    CLIENT HANDLER                                                          */
/******************************************************************************/
int client_handler(char * ip, bool is_udp, int pipe_read, int pipe_write, 
                   uint16_t port, int tcp_fd, bool DEBUG, int pipe_num);


#endif
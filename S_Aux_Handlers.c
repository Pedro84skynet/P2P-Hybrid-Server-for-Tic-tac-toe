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

#define _GNU_SOURCE         
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <stdbool.h>
#include <unistd.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/time.h>
#include <signal.h>

/*man pool*/
#include <poll.h>

#include "S_Aux_Handlers.h"
#include "DB_Manag_Sys.h"
#include "Protocol.h"

/*signal to log close*/
void handler_close(int sig)
{
    printf("\nServer shut down!\n");
    log_event("Server shut down!");
    exit(0);
}


/**************************************************************************************/
/*                                                                                    */
/*    MASTER HANDLER                                                                  */
/*                                                                                    */
/**************************************************************************************/
int master_handler(int player_rd[128][2], char * client_message, bool DEBUG, int this_pipe)
{
    if(DEBUG) printf("[Master handler] receive: %s\n", client_message);
    unsigned char client_message_copy[64];
    unsigned char processed_message[128];
    unsigned char * user, * pass, * command, * token, * old_pass, * new_pass;
    unsigned char * ip, *ip_p1, *ip_p2; 
    int pipe_num;
    unsigned char * winner, * loser; 
    char event[128];
    
    strncpy(client_message_copy, client_message, strlen(client_message));
    client_message_copy[strlen(client_message)] = '\0';
    command = strtok(client_message_copy, " ");
    if(DEBUG)printf("[Master handler]  command: %s\n", command);
/*  NEW    ___________________________________________________________________*/
    if (!strncmp(command, "new", 3)) 
    {
        if(DEBUG)printf("[Master handler]  receive NEW\n");
        log_event("Received request for new user");
        user = strtok(NULL, " ");
        pass = strtok(NULL, " ");
        if(DEBUG)printf("[Master handler] user:%s len: %zu\n", user, strlen(user));
        if(DEBUG)printf("[Master handler] pass:%s len: %zu\n", pass, strlen(pass));
        if(insert_user(user, pass)) 
        {
            printf("Error: new user NOT created.\n");
            write(player_rd[this_pipe][1], NACK_new_user, sizeof(NACK_new_user));
        }
        else 
        {
            sprintf(event,"new user %s created.", user);
            log_event(event);
            if(DEBUG) printf("[Master handler] new user created.\n");
            write(player_rd[this_pipe][1], ACK_new_user, sizeof(ACK_new_user));
        }
        return -1; 
    }
/*  PASS    __________________________________________________________________*/
    else if (!strncmp(command, "pass", 4)) 
    {
        log_event("[Master handler] Received request for new password");
        old_pass = strtok(NULL, " ");
        new_pass = strtok(NULL, " ");
        user = strtok(NULL, " ");
        user[strlen(user)] = '\0';
        if(DEBUG)printf("[Master handler] user:%s len: %zu\n", user, strlen(user));
        if (change_pass(user, old_pass, new_pass))
        {
            if(DEBUG) printf("Error: new password NOT created.\n");
            log_event("Error: new password NOT created.");
            write(player_rd[this_pipe][1], NACK_newpass_user, sizeof(NACK_newpass_user));
        }
        else
        {
            sprintf(event,"new password for user %s created.", user);
            log_event(event);
            if(DEBUG) printf("Master: new password created.\n");
            write(player_rd[this_pipe][1], ACK_newpass_user, sizeof(ACK_newpass_user));
        }
        return -1;
    }
/*  IN    ____________________________________________________________________*/
    else if (!strncmp(command, "in", 2)) 
    {
        if(DEBUG) printf("[Master handler] receive IN\n");
        log_event("log requested.");
        user = strtok(NULL, " ");
        pass = strtok(NULL, " ");
        ip = strtok(NULL, " ");
        pipe_num = atoi(strtok(NULL, " "));
        if(DEBUG) printf("[Master handler] user:%s len: %zu\n", user, strlen(user));
        if(DEBUG) printf("[Master handler] pass:%s len: %zu\n", pass, strlen(pass));
        if (!is_online(user))
        {
            if(log_user(user, pass, ip, pipe_num)) 
            {
                printf("Error: username or password did NOT match.\n");
                log_event("log denied: username or password did NOT match.");
                write(player_rd[this_pipe][1], NACK_in_user, sizeof(NACK_in_user));
            }
            else 
            {
                sprintf(event,"user %s logged. ip: %s", user, ip);
                log_event(event);
                if(DEBUG) printf("[Master handler] user logged.\n");
                write(player_rd[this_pipe][1], ACK_in_user, sizeof(ACK_in_user));
            }
        } 
        else
        {
            write(player_rd[this_pipe][1], NACK_already_logged, sizeof(NACK_already_logged));
        }
        return -1; 
    }
    /*    already_logged    */
    else if (!strncmp(command, NACK_already_logged, sizeof(NACK_already_logged)))
    {
        user = strtok(NULL, " ");
        if (is_online(user))
        {
            write(player_rd[this_pipe][1], ACK_in_user, sizeof(ACK_in_user));
        }
        else
        {
            write(player_rd[this_pipe][1], NACK_not_logged, sizeof(NACK_not_logged));
        } 
    }
/*  HALLOFFAME    ____________________________________________________________*/
    else if (!strncmp(command, "halloffame", 10)) 
    {
        if(halloffame_sender(player_rd[this_pipe][1])) {
            printf("Error: hall of fame not available. Not sended.\n");
            log_event("Error: hall of fame not available. Not sended.\n");
            write(player_rd[this_pipe][1], NACK_hallofame, sizeof(NACK_hallofame));
        } 
        else
        {
            log_event("sending a hall of fame list.");
            usleep(50000);
            write(player_rd[this_pipe][1], ACK_hallofame, sizeof(ACK_hallofame));
        }
        return -1;
    }
/*  l    _____________________________________________________________________*/ 
    else if (!strncmp(command, "l", 1)) 
    {
        if(l_sender(player_rd[this_pipe][1])) {
            printf("Error: online list not available. Not sended.\n");
            log_event("Error: online list not available. Not sended.\n");
            write(player_rd[this_pipe][1], NACK_online_l, sizeof(NACK_online_l));
        } 
        else
        {
            log_event("sending a online list.");
            usleep(50000);
            write(player_rd[this_pipe][1], ACK_online_l, sizeof(ACK_online_l));
        }
        return -1;
    }
/*  CALL    __________________________________________________________________*/
    else if (!strncmp(command, "call", 4)) 
    {
        user = strtok(NULL, " ");
        token = strtok(NULL, " ");
        if (is_online(user))
        {
            sprintf(event,"sending call to %s from %s.", user, token);
            log_event(event);
            sprintf(processed_message, "%s %s", client_message, what_ip(token));
            processed_message[strlen(processed_message)] = '\0';
            write(player_rd[what_pipe(user)][1], processed_message, strlen(processed_message));
            return 6; 
        }
        else
        {
            sprintf(event,"call received but %s is not online.", user);
            log_event(event);
            write(player_rd[this_pipe][1], NACK_online, sizeof(NACK_online));
            return 7;
        }
    }
    /*  ACK_accept  */
    else if (!strncmp(command, ACK_accept, sizeof(ACK_accept))) 
    {
        
        user = strtok(NULL, " ");
        token = strtok(NULL, " ");
        ip_p1 = what_ip(user);
        ip_p2 = what_ip(token);
        if(change_data(user, 3, NULL, 0))
        {
            sprintf(event,"Error: Database failed to logged out user %s.", user);
            log_event(event);
        }
        else if(change_data(token, 3, NULL, 0))
        {
            sprintf(event,"Error: Database failed to logged out user %s.", token);
            log_event(event);
        }
        else
        {
            sprintf(event,"user %s (ip: %s) is now playing with %s (ip: %s).", 
                           user, ip_p1, token, ip_p2);
            log_event(event);
        }
        free(ip_p1);
        free(ip_p2);
        write(player_rd[what_pipe(token)][1], ACK_accept, sizeof(ACK_accept));
        return 8;
    }
/*  OVER    __________________________________________________________________*/
    else if (!strncmp(command, "over", 4)) 
    {
        winner = strtok(NULL, " "); 
        loser = strtok(NULL, " ");
        if(DEBUG) printf("[Master handler] user %s has quitted\n", winner);
        sprintf(event,"Game Over: winner: user %s has quitted the game with %s", winner, loser);
        log_event(event);
        if(change_data(winner, 3, NULL, 0))
        {
            sprintf(event,"Error: Database failed quit game user %s.", token);
            log_event(event);
        }
        if(change_data(loser, 3, NULL, 0))
        {
            sprintf(event,"Error: Database failed quit game user %s.", token);
            log_event(event);
        }
        write(player_rd[this_pipe][1], Game_over, sizeof(Game_over));
        write(player_rd[what_pipe(loser)][1], Game_over, sizeof(Game_over));
    }
/*  OUT    ___________________________________________________________________*/
    else if (!strncmp(command, "out", 3)) 
    {
        user = strtok(NULL, " ");
        user[strlen(user)] = '\0';
        ip = what_ip(user);
        if(DEBUG) printf("[Master handler] user:%s len: %zu\n", user, strlen(user));
        if(change_data(user, 2, NULL, 0))
        {
            sprintf(event,"Error: Database failed to logged out user %s.", user);
            log_event(event);
            write(player_rd[this_pipe][1], NACK_out_user, sizeof(NACK_out_user));
        }
        else
        {
            sprintf(event,"user %s logged out. ip: %s", user, ip);
            log_event(event);
            write(player_rd[this_pipe][1], ACK_out_user, sizeof(ACK_out_user));
        }
        free(ip);
        return 10;
    }
/*  BYE    ___________________________________________________________________*/
    else if (!strncmp(command, "bye", 3)) 
    {
        user = strtok(NULL, " ");
        user[strlen(user)] = '\0';
        ip = what_ip(user);
        if(DEBUG) printf("[Master handler] user:%s len: %zu\n", user, strlen(user));
        if(change_data(user, 2, NULL, 0))
        {
            sprintf(event,"Error: Database failed to logged out user %s.", user);
            log_event(event);
            write(player_rd[this_pipe][1], NACK_out_user, sizeof(NACK_out_user));
        }
        else
        {
            sprintf(event,"user %s has disconnected from the server. ip: %s", user, ip);
            log_event(event);
        }
        free(ip);
        return 11;
    } 
/*  I_win    __________________________________________________________________*/
    else if (!strncmp(command, I_win, sizeof(I_win))) 
    {
        winner = strtok(NULL, " "); 
        loser = strtok(NULL, " ");
        ip_p1 = what_ip(user); 
        ip_p2 = what_ip(loser);
        if(DEBUG) printf("[Master handler] winner:%s (ip: %s) loser: %s (ip: %s)\n", 
                            winner, loser, ip_p1, ip_p2);
        sprintf(event,"[Master handler] winner:%s (ip: %s) loser: %s (ip: %s)\n", 
                            winner, loser, ip_p1, ip_p2);
        log_event(event);
        write(player_rd[this_pipe][1], You_won, sizeof(You_won));
        if(change_data(winner, 1, NULL, 0))
        {
            sprintf(event,"Error: Database failed add score to user %s.", user);
            log_event(event);
        }
        if(change_data(winner, 3, NULL, 0))
        {
            sprintf(event,"Error: Database failed quit game user %s.", token);
            log_event(event);
        }
        if(change_data(loser, 3, NULL, 0))
        {
            sprintf(event,"Error: Database failed quit game user %s.", token);
            log_event(event);
        }
        write(player_rd[what_pipe(loser)][1], You_lose, sizeof(You_lose));
        free(ip_p1);
        free(ip_p2);
    } 
/*  Game_over    ______________________________________________________________*/
/*  Draw    ___________________________________________________________________*/
    else if ((!strncmp(command, Game_over, sizeof(Game_over))) ||
             (!strncmp(command, Draw, sizeof(Draw)))) 
    {
        winner = strtok(NULL, " ");
        winner[strlen(winner)] = '\0';
        loser = strtok(NULL, " ");
        loser[strlen(loser)] = '\0';
        ip_p1 = what_ip(user); 
        ip_p2 = what_ip(loser);
        if(DEBUG) printf("[Master handler] draw between %s (ip: %s) and %s (ip: %s)\n", 
                            winner, loser, ip_p1, ip_p2);
        sprintf(event,"[Master handler] draw between %s (ip: %s) and %s (ip: %s)\n", 
                            winner, loser, ip_p1, ip_p2);
        log_event(event);
        if(change_data(winner, 3, NULL, 0))
        {
            sprintf(event,"Error: Database failed quit game user %s.", token);
            log_event(event);
        }
        if(change_data(loser, 3, NULL, 0))
        {
            sprintf(event,"Error: Database failed quit game user %s.", token);
            log_event(event);
        }
        write(player_rd[this_pipe][1], Draw, sizeof(Draw));
        write(player_rd[what_pipe(loser)][1], Draw, sizeof(Draw));
    }
    else if (!strncmp(command, Client_down, sizeof(Client_down)))
    {
        user = strtok(NULL, " ");
        sprintf(event,"Lost connection with user %s. (ip : %s)", user, what_ip(user));
        log_event(event);
    }
}


/**************************************************************************************/
/*                                                                                    */
/*    CLIENT HANDLER                                                                  */
/*                                                                                    */
/**************************************************************************************/
int client_handler(char * ip, bool is_udp, int pipe_read, int pipe_write, 
                    uint16_t port, int tcp_fd, bool DEBUG, int pipe_num) 
{
    int udp_fd;
    ssize_t n_bytes;
    socklen_t len;
    pid_t listener;
    pid_t sender;
    struct sockaddr_in addr;

    int listener_pipe[2]; // listener_pipe[0] <- Read; listener_pipe[1] <- Write;
    pipe(listener_pipe);
    int sender_pipe[2]; // sender_pipe[0] <- Read; sender_pipe[1] <- Write;
    pipe(sender_pipe);
    int list_to_send_pipe[2]; // sender_pipe[0] <- Read; sender_pipe[1] <- Write;
    pipe(list_to_send_pipe);

    int ret, ACK_NACK;
    bool logged = false;

    unsigned char client_message[64]; 
    unsigned char server_message[64]; 

    struct pollfd poll_fd[2];
    int timeout = 3*60*1000; //milésimos

    if(is_udp) 
    {
        memset((void *)&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port);
        addr.sin_addr.s_addr = htonl(INADDR_ANY);

        if ((udp_fd = socket(AF_INET, SOCK_DGRAM, 0)) == -1 )
        {
            printf("client_handler: Error: socket not created");
            exit(EXIT_FAILURE);
        }
        if (bind(udp_fd, (struct sockaddr *) &addr, sizeof(addr)) == -1)
        {
            printf("client_handler: Error: udp_fd bind failed");
            exit(EXIT_FAILURE);
        } 
    }
    else
    {
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port);
        addr.sin_addr.s_addr = htonl(INADDR_ANY);
    }
    
    
    len = sizeof(addr);

    if ((listener = fork()) == -1)
    {
        printf("client_handler: Erro: fork listener from udp_client_handler failed\n");
        exit(EXIT_FAILURE);
    }
    /*
        Listener
    */
    if (listener == 0) 
    {
        close(pipe_read);
        close(pipe_write);

        struct pollfd listen_poll_fd[1];
        int timeout = 3*60*1000; //milésimo
    
        if(is_udp)
        {
            n_bytes = recvfrom(udp_fd, (void *) client_message, sizeof(client_message), 0,
                                (struct sockaddr *) &addr, (socklen_t *) &len);
            if (n_bytes == -1)
            {
                printf("Erro: recvfrom from udp_client_handler failed\n");
                exit(EXIT_FAILURE);
            }
            // port = addr.sin_port;
            write(list_to_send_pipe[1], (void *) &addr, sizeof(addr));
            if(DEBUG) printf("[Listener] Porta do cliente UDP: %d\n", ntohs(addr.sin_port));
            client_message[n_bytes + 1] = '\0';
            if(DEBUG) printf("[Listener] recebeu do socket: %s\n", client_message);
            if(DEBUG) printf("[Listener]    n_bytes: %d\n", (int) n_bytes);
            if (strncmp(client_message, Ping, sizeof(Ping)))
            {
                write(listener_pipe[1], (void *) client_message, sizeof(client_message));
            }
        }
        n_bytes = 1;
        while (n_bytes)
        {
            if(is_udp) listen_poll_fd[0].fd = udp_fd;
            else listen_poll_fd[0].fd = tcp_fd;
            listen_poll_fd[0].events = POLLIN;

            ret = poll(listen_poll_fd, 1, timeout);
            if(ret == -1)
            {
                printf("Error: poll from handler failed");
                exit(EXIT_FAILURE);
            }
            if(ret == 0) // Client is not responding!
            {
                printf("\n...Client is not responding!\n"); 
                write(listener_pipe[1], (void *) Client_down, sizeof(Client_down));
            }
            if ((listen_poll_fd[0].revents & POLLIN) && 
                ((listen_poll_fd[0].fd == udp_fd)    ||
                 (listen_poll_fd[0].fd == tcp_fd)))
            {
                memset((void *)client_message, 0, sizeof(client_message));
                if(is_udp)
                {
                    n_bytes = recvfrom(udp_fd, (void *) client_message, sizeof(client_message), 0,
                                        (struct sockaddr *) &addr, (socklen_t *) &len);
                }
                else
                {
                    n_bytes = recv(tcp_fd, (void *) client_message, sizeof(client_message), 0);
                }
                if (n_bytes == -1)
                {
                    printf("Erro: receive from client_handler failed\n");
                    exit(EXIT_FAILURE);
                }
                client_message[n_bytes + 1] = '\0';
                if(DEBUG) printf("[Listener] recebeu do socket: %s\n", client_message);
                if(DEBUG) printf("[Listener]    n_bytes: %d\n", (int) n_bytes);
                if (strncmp(client_message, Ping, sizeof(Ping)))
                {   
                    if(DEBUG) printf("[Listener] recebeu do socket: %s\n", client_message);
                    write(listener_pipe[1], (void *) client_message, strlen(client_message));
                } 
            }
        }
        exit(EXIT_SUCCESS);
    }

    if ((sender = fork()) == -1)
    {
        printf("Erro: fork sender from udp_client_handler failed\n");
        exit(EXIT_FAILURE);
    }
    /*
        Sender
    */
    if (sender == 0) 
    {
        close(pipe_read);
        close(pipe_write);

        struct pollfd sender_poll_fd[1];
        int timeout = 15*1000; //milésimo

        if (is_udp)
        {
            read(list_to_send_pipe[0], (void *) &addr, sizeof(addr));
            //addr.sin_port = port;
            if(DEBUG) printf("[Sender] Porta do cliente UDP: %d\n", ntohs(addr.sin_port));
        }
        n_bytes = 1;
        while (n_bytes)
        {
            sender_poll_fd[0].fd = sender_pipe[0];
            sender_poll_fd[0].events = POLLIN;

            ret = poll(sender_poll_fd, 1, timeout);
            if(ret == -1)
            {
                printf("Error: poll from sender failed");
                exit(EXIT_FAILURE);
            }
            if(ret == 0)
            {
                if (is_udp)
                {
                    n_bytes = sendto(udp_fd, (void *) Ping, sizeof(Ping), 0,
                        (struct sockaddr *) &addr, (socklen_t ) sizeof(addr));
                    if (n_bytes == -1)
                    {
                        printf("Erro: sendto failed\n");
                        exit(EXIT_FAILURE);
                    }
                } 
                else 
                {
                    n_bytes = send(tcp_fd, (void *) Ping, sizeof(Ping), 0);
                    if (n_bytes == -1)
                    {
                        printf("Erro: send failed\n");
                        exit(EXIT_FAILURE);
                    }
                }
            }
            if ((sender_poll_fd[0].revents & POLLIN) &&
                (sender_poll_fd[0].fd == sender_pipe[0]))
            {
                read(sender_pipe[0], (void *) server_message, (size_t) sizeof(server_message));
                if(DEBUG) printf("[Sender] Sender recebeu do pipe: %s\n", server_message);
                if (is_udp)
                {
                    n_bytes = sendto(udp_fd, (void *) server_message, strlen(server_message), 0,
                                (const struct sockaddr *) &addr, (socklen_t ) sizeof(addr));
                }
                else
                {
                    n_bytes = send(tcp_fd, (void *) server_message, strlen(server_message), 0);
                }
                if (n_bytes == -1)
                {
                    printf("Erro: sender from client_handler failed\n");
                    exit(EXIT_FAILURE);
                }
                if(DEBUG) printf("[Sender]   .... Sender enviou mensagem para Cliente\n");
                if(DEBUG) printf("[Sender]    n_bytes: %d\n", (int) n_bytes);
                memset((void *)server_message, 0, sizeof(server_message));
            }
        }
        exit(EXIT_SUCCESS);
    }
    else // Is Processor 
    {
        close(udp_fd);
        // Auxiliars
        unsigned char client_message_copy[64], username[32];
        unsigned char client_message_processed[128], server_message_processed[128];
        unsigned char * user, * pass, * command, * token, * old_pass, * new_pass;

        while(1) 
        {
            poll_fd[0].fd = listener_pipe[0];
            poll_fd[0].events = POLLIN;
            poll_fd[1].fd = pipe_read;
            poll_fd[1].events = POLLIN;

            memset((void *)client_message, 0, 64);
            ret = poll(poll_fd, 2, -1);
            if (ret == -1)
            {
                printf("Error: poll from handler failed\n");
                exit(EXIT_FAILURE);
            }
        /*    
            Processa mensagem do listener, return client_message;    
        */
            else if ((poll_fd[0].revents == POLLIN) && (poll_fd[0].fd == listener_pipe[0]))
            {
                memset(client_message, 0, sizeof(client_message)); 
                memset(client_message_copy, 0, sizeof(client_message_copy)); 
                read(listener_pipe[0], (void *) client_message, sizeof(client_message));
                if(DEBUG) printf("[client_handler Main] Processador recebeu do listener: %s len: %zu\n", 
                                    client_message, strlen(client_message));
                strncpy(client_message_copy, client_message, strlen(client_message));
                client_message_copy[strlen(client_message)] = '\0';
                command = strtok(client_message_copy, " ");
                if(DEBUG) printf("[client_handler Main] Processador recebeu do listener: %s len: %zu\n", 
                                    client_message, strlen(client_message));
            /*  Specials Cases    */ 
            /*  already_logged    */
                if (!strncmp(command, NACK_already_logged, sizeof(NACK_already_logged)))
                {
                    user =  strtok(NULL, " ");
                    memset(username, 0, sizeof(username)); 
                    strncpy(username, user, strlen(user));
                    username[strlen(user)] = '\0';
                    write(pipe_write, (void *) client_message, strlen(client_message)); 
                    read(pipe_read, (void *) server_message, sizeof(server_message));
                    server_message[strlen(server_message)] = '\0';
                    command = strtok(server_message, " ");
                    if (!strncmp(command, ACK_in_user, sizeof(ACK_in_user)))
                    {
                        write(listener_pipe[1], ACK_in_user, sizeof(ACK_in_user));
                        logged = true;
                    }
                    else 
                    {
                        write(listener_pipe[1], NACK_not_logged, sizeof(NACK_not_logged));
                        logged = false;
                    }
                }
            /*  IN  */
            /*  Must record username in variable username before send to master.                      */
                else if (!strncmp(command, "in", 3))
                {
                    if (logged)
                    {
                        write(sender_pipe[1], (void *) NACK_already_logged, sizeof(NACK_already_logged));
                    }
                    else 
                    {
                        printf("    client_message: %s len: %zu\n", client_message, strlen(client_message));
                        user = strtok(NULL, " "); 
                        if(DEBUG) printf("[client_handler Main] user:%s len: %zu\n", user, strlen(user));
                        memset(username, 0, sizeof(username)); 
                        printf("    client_message: %s len: %zu\n", client_message, strlen(client_message));
                        strncpy(username, user, strlen(user));
                        printf("    client_message: %s len: %zu\n", client_message, strlen(client_message));
                        username[strlen(user)] = '\0';
                        if(DEBUG) printf("[client_handler Main] username: %s len %zu\n", username, strlen(username));
                        memset(client_message_processed, 0, sizeof(client_message_processed)); 
                        printf("    client_message: %s len: %zu\n", client_message, strlen(client_message));
                        printf("    ip: %s len: %zu\n", ip, strlen(ip));
                        printf("    pipe_num: %d\n", pipe_num);
                        sprintf(client_message_processed, "%s %s %d", client_message, ip, pipe_num); 
                        printf("    client_message: %s len: %zu\n", client_message, strlen(client_message));
                        printf("    ip: %s len: %zu\n", ip, strlen(ip));
                        printf("    pipe_num: %d\n", pipe_num);
                        client_message_processed[strlen(client_message_processed)] = '\0';
                        if(DEBUG) printf("[client_handler Main] client_message_processed: %s\n", client_message_processed);
                        if(DEBUG) printf("writing on pipe_write\n");
                        write(pipe_write, (void *) client_message_processed, strlen(client_message_processed));
                    }
                }
            /*  PASS  */
            /*  CALL  */
            /*  OUT   */
            /*  Must concatenate request with username before send to master.                         */
                else if (!strncmp(command, "pass", 4) ||
                         !strncmp(command, "call", 4) || 
                         !strncmp(command, "out" , 3))
                {
                    if(logged) 
                    {
                        sprintf(client_message_processed, "%s %s", client_message, username); 
                        client_message_processed[strlen(client_message_processed)] = '\0';
                        if(DEBUG) printf("[client_handler Main] client_message_processed: %s len: %zu\n", 
                                          client_message_processed, 
                                          strlen(client_message_processed));
                        write(pipe_write, (void *) client_message_processed, strlen(client_message_processed)); 
                        memset(client_message_processed, 0, sizeof(client_message_processed));
                    } 
                    else
                    { 
                        write(sender_pipe[1], (void *) NACK_not_logged, (size_t) sizeof(NACK_not_logged));
                    }
                }
            /*  BYE  */
            /*  Must concatenate request with username in logged case before send to master.          */
            /*  kill sender and listener before exit.                                                 */
                else if (!strncmp(command, "bye", 3))
                {
                    if(logged) 
                    {
                        sprintf(client_message_processed, "%s %s", client_message, username); 
                        client_message_processed[strlen(client_message_processed)] = '\0';
                        write(pipe_write, (void *) client_message_processed, 
                                (size_t) strlen(client_message_processed));
                        memset(client_message_processed, 0, sizeof(client_message_processed));
                        logged = false;
                    }
                    write(sender_pipe[1], (void *) ACK_bye_user, (size_t) sizeof(ACK_bye_user));
                    sleep (1);
                    kill (sender, SIGKILL);
                    kill (listener, SIGKILL);
                    return 0;
                }
            /*  L           */
            /*  HALLOFFAME  */
            /*  Must be logged to request.                                                            */
                else if (!strncmp(command, "l", 1) || 
                         !strncmp(command, "halloffame" , 10))
                {
                    if(logged) 
                    {
                        write(pipe_write, (void *) client_message, (size_t) sizeof(client_message));
                    }
                    else
                    {
                        write(sender_pipe[1], (void *) NACK_not_logged, (size_t) sizeof(NACK_not_logged));
                    }
                }
                else
                {
                    write(pipe_write, (void *) client_message, (size_t) sizeof(client_message));
                }
            }

        /*
            Processa mensagem do main do servidor, return server_message;  
        */
            else if ((poll_fd[1].revents == POLLIN) && (poll_fd[1].fd == pipe_read))
            {
                memset(server_message, 0, sizeof(server_message));
                read(pipe_read, (void *) server_message, sizeof(server_message));
                if(DEBUG) printf("[client_handler Main] Processador recebeu do main: %s\n", server_message);
            /*  logged  */
                if (!strncmp(server_message, ACK_in_user, sizeof(ACK_in_user)))
                {
                    logged = true;
                }
                else if (!strncmp(server_message, ACK_out_user, sizeof(ACK_out_user)))
                {
                    logged = false;
                }
                write(sender_pipe[1], (void *) server_message, sizeof(server_message));
            }
        }
    }  
}

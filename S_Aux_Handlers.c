/******************************************************************************
 *  Compilation:  (Use make)
 *  Execution:    ./EP2_Server port_number protocol
 *
 *  - port_number: port number used to connect in EP2_Servidor.
 *  - protocol   : "UDP" or "TCP"
 *
 *  DESCRIPTION
 *
 *  Auxiliars Functions for the server to handle requests
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

char ACK_new_user[21]        = "...new user created!";
char NACK_new_user[19]       = "...new user failed";
char ACK_in_user[11]         = "...logged!";
char NACK_in_user[47]        = "...not logged, username or password incorrect ";
char ACK_newpass_user[25]    = "...new password created!";
char NACK_newpass_user[35]   = "...error: new password not created";
char ACK_out_user[15]        = "...logged out!";
char NACK_out_user[23]       = "...error: still logged";
char ACK_bye_user[8]         = "...bye!";
char NACK_already_logged[19] = "...Already Logged!";
char NACK_not_logged[26]     = "...you need to be logged!";
char ACK_hallofame[21]       = "********************";
char NACK_hallofame[30]      = "...hall of fame not available";
char ACK_online_l[13]        = "...have fun!";
char NACK_online_l[29]       = "...online list not available";

char Ping[9]                 = "...ping";

/*****************************************************************************************************/
/*                                                                                                   */
/*    MASTER HANDLER                                                                                 */
/*                                                                                                   */
/*****************************************************************************************************/
void master_handler(int player_rd, char * client_message)
{
    printf("master_handler receive: %s\n", client_message);
    unsigned char client_message_copy[64];
    unsigned char * user, * pass, * command, * token, * old_pass, * new_pass;
    char event[64];
    
    strncpy(client_message_copy, client_message, strlen(client_message));
    client_message_copy[strlen(client_message)] = '\0';
    command = strtok(client_message_copy, " ");
/*  NEW    ___________________________________________________________________*/
    if (!strncmp(command, "new", 3)) 
    {
        printf("Master: receive NEW\n");
        log_event("Received request for new user");
        user = strtok(NULL, " ");
        pass = strtok(NULL, " ");
        printf("Master: [user:%s len: %ld]\n", user, strlen(user));
        printf("Master: [pass:%s len: %ld]\n", pass, strlen(pass));
        if(insert_user(user, pass)) 
        {
            printf("Error: new user NOT created.\n");
            write(player_rd, NACK_new_user, sizeof(NACK_new_user));
        }
        else 
        {
            sprintf(event,"new user %s created.", user);
            log_event(event);
            printf("Master: new user created.\n");
            write(player_rd, ACK_new_user, sizeof(ACK_new_user));
        } 
    }
/*  PASS    __________________________________________________________________*/
    else if (!strncmp(command, "pass", 4)) 
    {
        log_event("Received request for new password");
        old_pass = strtok(NULL, " ");
        new_pass = strtok(NULL, " ");
        user = strtok(NULL, " ");
        user[strlen(user)] = '\0';
        printf("[user:%s len: %ld]\n", user, strlen(user));
        if (change_pass(user, old_pass, new_pass))
        {
            printf("Error: new password NOT created.\n");
            log_event("Error: new password NOT created.");
            write(player_rd, NACK_newpass_user, sizeof(NACK_newpass_user));
        }
        else
        {
            sprintf(event,"new password for user %s created.", user);
            log_event(event);
            printf("Master: new password created.\n");
            write(player_rd, ACK_newpass_user, sizeof(ACK_newpass_user));
        }
    }
/*  IN    ____________________________________________________________________*/
    else if (!strncmp(command, "in", 2)) 
    {
        printf("Master: receive IN\n");
        log_event("log requested.");
        user = strtok(NULL, " ");
        pass = strtok(NULL, " ");
        printf("Master: [user:%s len: %ld]\n", user, strlen(user));
        printf("Master: [pass:%s len: %ld]\n", pass, strlen(pass));
        if(log_user(user, pass)) {
            printf("Error: username or password did NOT match.\n");
            log_event("log denied: username or password did NOT match.");
            write(player_rd, NACK_in_user, sizeof(NACK_in_user));
        }
        else 
        {
            sprintf(event,"user %s logged.", user);
            log_event(event);
            printf("Master: user logged.\n");
            write(player_rd, ACK_in_user, sizeof(ACK_in_user));
        }
        int change_data(char *username, int cod); 
    }
/*  HALLOFFAME    ____________________________________________________________*/
    else if (!strncmp(command, "halloffame", 10)) 
    {
        if(halloffame_sender(player_rd)) {
            printf("Error: hall of fame not available. Not sended.\n");
            log_event("Error: hall of fame not available. Not sended.\n");
            write(player_rd, NACK_hallofame, sizeof(NACK_hallofame));
        } 
        else
        {
            log_event("sending a hall of fame list.");
            write(player_rd, ACK_hallofame, sizeof(ACK_hallofame));
        }
    }
/*  l    _____________________________________________________________________*/ 
    else if (!strncmp(command, "l", 1)) 
    {
        if(l_sender(player_rd)) {
            printf("Error: online list not available. Not sended.\n");
            log_event("Error: online list not available. Not sended.\n");
            write(player_rd, NACK_online_l, sizeof(NACK_online_l));
        } 
        else
        {
            log_event("sending a online list.");
            write(player_rd, ACK_online_l, sizeof(ACK_online_l));
        }
    }
/*  CALL    __________________________________________________________________*/
    else if (!strncmp(command, "pass", 4)) 
    {
        /* code */
    }
/*  PLAY    __________________________________________________________________*/
    else if (!strncmp(command, "play", 4)) 
    {
        /* code */
    }
/*  DELAY    _________________________________________________________________*/
    else if (!strncmp(command, "delay", 5)) 
    {
        /* code */
    }
/*  OVER    __________________________________________________________________*/
    else if (!strncmp(command, "over", 4)) 
    {
        /* code */
    }
/*  OUT    ___________________________________________________________________*/
    else if (!strncmp(command, "out", 3)) 
    {
        user = strtok(NULL, " ");
        user[strlen(user)] = '\0';
        printf("[user:%s len: %ld\n", user, strlen(user));
        if(change_data(user, 2))
        {
            sprintf(event,"Error: Database failed to logged out user %s.", user);
            log_event(event);
            write(player_rd, NACK_out_user, sizeof(NACK_out_user));
        }
        else
        {
            sprintf(event,"user %s logged out.", user);
            log_event(event);
            write(player_rd, ACK_out_user, sizeof(ACK_out_user));
        }
    }
/*  BYE    ___________________________________________________________________*/
    else if (!strncmp(command, "bye", 3)) 
    {
        user = strtok(NULL, " ");
        user[strlen(user)] = '\0';
        printf("[user:%s len: %ld\n", user, strlen(user));
        if(change_data(user, 2))
        {
            sprintf(event,"Error: Database failed to logged out user %s.", user);
            log_event(event);
            write(player_rd, NACK_out_user, sizeof(NACK_out_user));
        }
    }  
}


/*****************************************************************************************************/
/*                                                                                                   */
/*    CLIENT HANDLER                                                                                 */
/*                                                                                                   */
/*****************************************************************************************************/
int client_handler(bool is_udp, int pipe_read, int pipe_write, uint16_t port, int tcp_fd) 
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

    unsigned char ping[1] = {1}; 
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
            printf("Error: socket not created");
            exit(EXIT_FAILURE);
        }
        if (bind(udp_fd, (struct sockaddr *) &addr, sizeof(addr)) == -1)
        {
            printf("Error: udp_fd bind failed");
            exit(EXIT_FAILURE);
        } 
    }
    
    len = sizeof(addr);

    if ((listener = fork()) == -1)
    {
        printf("Erro: fork listener from udp_client_handler failed\n");
        exit(EXIT_FAILURE);
    }
    if (listener == 0) // Is listener
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
            port = addr.sin_port;
            write(list_to_send_pipe[1],(void *) &port, sizeof(port));
            printf("Listener: Porta do cliente UDP: %d\n", ntohs(addr.sin_port));
            client_message[n_bytes + 1] = '\0';
            printf("Listener recebeu do socket: %s\n", client_message);
            printf("    n_bytes: %d\n", (int) n_bytes);
            if (strncmp(client_message, Ping, sizeof(Ping)))
            {
                write(listener_pipe[1], (void *) client_message, (size_t) sizeof(client_message));
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
                printf("Listener recebeu do socket: %s\n", client_message);
                printf("    n_bytes: %d\n", (int) n_bytes);
                if (strncmp(client_message, Ping, sizeof(Ping)))
                {   
                    printf("Listener recebeu do socket: %s\n", client_message);
                    write(listener_pipe[1], (void *) client_message, (size_t) sizeof(client_message));
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
    if (sender == 0) // Is sender
    {
        close(pipe_read);
        close(pipe_write);

        struct pollfd sender_poll_fd[1];
        int timeout = 15*1000; //milésimo

        if (is_udp)
        {
            read(list_to_send_pipe[0], (void *) &port, sizeof(port));
            addr.sin_port = port;
            printf("Sender: Porta do cliente UDP: %d\n", ntohs(addr.sin_port));
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
                printf("Sender recebeu do pipe: %s\n", server_message);
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
                printf("   .... Sender enviou mensagem para Cliente\n");
                printf("    n_bytes: %d\n", (int) n_bytes);
                memset((void *)server_message, 0, sizeof(server_message));
            }
        }
        exit(EXIT_SUCCESS);
    }
    else // Is Processor 
    {
        close(udp_fd);
        // Auxiliars
        unsigned char client_message_copy[64], client_message_processed[128], username[64];
        unsigned char * user, * pass, * command, * token, * old_pass, * new_pass;
        bool logged = false;

        while(1) 
        {
            poll_fd[0].fd = listener_pipe[0];
            poll_fd[0].events = POLLIN;
            poll_fd[1].fd = pipe_read;
            poll_fd[1].events = POLLIN;

            memset((void *)client_message, 0, 64);
            ret = poll(poll_fd, 2, 0);
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
                read(listener_pipe[0], (void *) client_message, (size_t) sizeof(client_message));
                printf("Processador recebeu do listener: %s\n", client_message);
                
                strncpy(client_message_copy, client_message, strlen(client_message));
                client_message_copy[strlen(client_message)] = '\0';
                command = strtok(client_message_copy, " ");
            /*  Specials Cases*/
            /*  IN    ________________________________________________________________________________*/
            /*  Must record username in variable username before send to master.                      */
                if (!strncmp(command, "in", 4))
                {
                    if (logged)
                    {
                        write(sender_pipe[1], (void *) NACK_already_logged, (size_t) sizeof(NACK_already_logged));
                    }
                    else 
                    {
                        user = strtok(NULL, " ");
                        printf("Processor: [user:%s len: %ld]\n", user, strlen(user));
                        memset(username, 0, 64); 
                        strncpy(username, user, strlen(user));
                        username[strlen(username)] = '\0';
                        printf("[username: %s len %ld]\n", username, strlen(username));
                        write(pipe_write, (void *) client_message, (size_t) sizeof(client_message));
                    }
                }
            /*  PASS   _______________________________________________________________________________*/
            /*  OUT    _______________________________________________________________________________*/
            /*  Must concatenate request with username before send to master.                         */
                else if (!strncmp(command, "pass", 4) || 
                         !strncmp(command, "out" , 3))
                {
                    if(logged) 
                    {
                        sprintf(client_message_processed, "%s %s", client_message, username); 
                        client_message_processed[strlen(client_message_processed)] = '\0';
                        printf("[client_message_processed: %s len: %ld]\n", 
                                    client_message_processed, 
                                    strlen(client_message_processed));
                        write(pipe_write, (void *) client_message_processed, 
                                (size_t) strlen(client_message_processed)); 
                        memset(client_message_processed, 0, sizeof(client_message_processed));
                    } 
                    else
                    { 
                        write(sender_pipe[1], (void *) NACK_not_logged, (size_t) sizeof(NACK_not_logged));
                    }
                }
            /*  BYE    _______________________________________________________________________________*/
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
            /*  L    _________________________________________________________________________________*/
            /*  HALLOFFAME    ________________________________________________________________________*/
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
                memset(client_message, 0, sizeof(client_message)); 
                memset(client_message_copy, 0, sizeof(client_message_copy)); 
            }
            /*
                Processa mensagem do main do servidor, return server_message;
            */
            else if ((poll_fd[1].revents == POLLIN) && (poll_fd[1].fd == pipe_read))
            {
                read(pipe_read, (void *) server_message, (size_t) sizeof(server_message));
                printf("Processador  recebeu do main: %s\n", server_message);
                if (!strncmp(server_message, ACK_in_user, sizeof(ACK_in_user)))
                {
                    logged = true;
                }
                else if (!strncmp(server_message, ACK_out_user, sizeof(ACK_out_user)))
                {
                    logged = false;
                }
                write(sender_pipe[1], (void *) server_message, (size_t) sizeof(server_message));
                memset(server_message, 0, sizeof(server_message));
            }
        }
    }  
}

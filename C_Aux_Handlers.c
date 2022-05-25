/******************************************************************************
 *  Compilation:  (Use make)
 *  Execution:    ./EP2_Client port_number protocol
 *
 *  - port_number: port number used to connect in EP2_Servidor.
 *  - protocol   : "UDP" or "TCP"
 *
 *  DESCRIPTION
 *
 *  handles for clients process for listening, sending and read/write input for
 *  user (front-end)
 *
 *  PROJECT DECISIONS OR UNFINISHED TASKS (?)
 *
 *  List them bellow
 *
 *  - 
 *  -
 *
 ******************************************************************************/

#define _GNU_SOURCE 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <poll.h>
#include <signal.h>

#include "C_Aux_Handlers.h"
#include "Hash_Game.h"

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
char Reconnect[36]           = "...starting reconnection procedure.";
char Server_down[32]         = "...lost connection with server.";

pid_t listener_process(int client_sockfd, bool is_udp,
                        struct sockaddr_in * serv_addr,
                        int listener_pipe) 
{
    pid_t listener;
    int n_bytes, len, ret; 
    char server_message[64];

    if ((listener = fork()) == -1)
    {
        printf("Erro: fork listener failed\n");
        exit(EXIT_FAILURE);
    }
    if (listener == 0) 
    {
        len = sizeof(struct sockaddr_in);
        struct pollfd listen_poll_fd[1];
        int timeout = 3*60*1000; // 3 minutos

        n_bytes = 1;
        while (1) 
        {
            listen_poll_fd[0].fd = client_sockfd;
            listen_poll_fd[0].events = POLLIN;

            ret = poll(listen_poll_fd, 1, timeout);
            if(ret == -1)
            {
                printf("Error: poll from handler failed");
                exit(EXIT_FAILURE);
            }
            if(ret == 0) //Server_down
            {
                printf("\nListener não recebeu nenhama conexão do servidor\n");
                close(client_sockfd);
                write(listener_pipe, (void *) Server_down, (size_t) sizeof(Server_down));
                sleep(10);
                return 0;
            }
            if ((listen_poll_fd[0].revents & POLLIN) && 
                (listen_poll_fd[0].fd == client_sockfd))
            {
                if (is_udp)
                {
                    n_bytes = recvfrom(client_sockfd, (void *) server_message, sizeof(server_message), 0,
                        (struct sockaddr *) serv_addr, (socklen_t *) &len);
                    if (n_bytes == -1)
                    {
                        printf("Erro: recvfrom failed\n");
                        exit(EXIT_FAILURE);
                    }
                } 
                else 
                {
                    n_bytes = recv(client_sockfd, (void *) server_message, sizeof(server_message), 0);
                    if (n_bytes == -1)
                    {
                        printf("Erro: recv failed\n");
                        exit(EXIT_FAILURE);
                    }
                    if (!n_bytes)
                    {
                        printf("\nServidor fechou a conexão tcp!\n");
                        close(client_sockfd);
                        write(listener_pipe, (void *) Reconnect, (size_t) sizeof(Reconnect));
                        return 0;
                    }
                }
                server_message[n_bytes] = '\0';
                printf("Listener recebeu do socket: %s\n", server_message);
                printf("    n_bytes: %d\n", (int) n_bytes);
                if (strncmp(server_message, Ping, sizeof(Ping)))
                {
                    write(listener_pipe, (void *) server_message, (size_t) sizeof(server_message));   
                }
                memset((void *)server_message, 0, sizeof(server_message));
            }
        }
        exit(EXIT_SUCCESS);
    }
    return listener;
}

pid_t sender_process(int client_sockfd, bool is_udp,
                        struct sockaddr_in * serv_addr,
                        int sender_pipe) 
{
    pid_t sender;
    int n_bytes, len, ret; 
    char client_message[64];
    
    if ((sender = fork()) == -1)
    {
        printf("Erro: fork sender failed\n");
        exit(EXIT_FAILURE);
    }
    if (sender == 0) 
    {
        struct pollfd sender_poll_fd[1];
        int timeout = 15*1000; // 15 segundos para um ping

        n_bytes = 1;
        while (n_bytes)
        {
            sender_poll_fd[0].fd = sender_pipe;
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
                    n_bytes = sendto(client_sockfd, (void *) Ping, sizeof(Ping), 0,
                        (struct sockaddr *) serv_addr, (socklen_t ) sizeof(struct sockaddr_in));
                    if (n_bytes == -1)
                    {
                        printf("Error: sendto failed\n");
                        exit(EXIT_FAILURE);
                    }
                } 
                else 
                {
                    n_bytes = send(client_sockfd, (void *) Ping, sizeof(Ping), 0);
                    if (n_bytes == -1)
                    {
                        printf("Error: send failed\n");
                        exit(EXIT_FAILURE);
                    }
                }
            }
            if ((sender_poll_fd[0].revents & POLLIN) && 
                (sender_poll_fd[0].fd = sender_pipe))
            {
                memset((void *)client_message, 0, sizeof(client_message));
                read(sender_pipe, (void *) client_message, (size_t) sizeof(client_message));
                printf("Sender recebeu do pipe: %s\n", client_message);

                if (is_udp)
                {
                    n_bytes = sendto(client_sockfd, (void *) client_message, strlen(client_message), 0,
                        (struct sockaddr *) serv_addr, (socklen_t ) sizeof(struct sockaddr_in));
                    if (n_bytes == -1)
                    {
                        printf("Erro: sendto failed\n");
                        exit(EXIT_FAILURE);
                    }
                } 
                else 
                {
                    n_bytes = send(client_sockfd, (void *) client_message, strlen(client_message), 0);
                    if (n_bytes == -1)
                    {
                        printf("Erro: send failed\n");
                        exit(EXIT_FAILURE);
                    }
                }
                printf("   .... Sender enviou mensagem para Servidor\n");
                printf("    n_bytes: %d\n", (int) n_bytes);
            }
        }
        exit(EXIT_SUCCESS);
    }
    return sender;
}

pid_t front_end_process(int back_end_pipe, int front_end_pipe) 
{
    pid_t  front_end;
    int n_bytes, len, ret; 
    char user_input[64], user_input_copy[64], server_message[64], client_message[64];
    char * command;
    bool logged = false;

    if ((front_end = fork()) == -1)
    {
        printf("Erro: fork front_end failed\n");
        exit(EXIT_FAILURE);
    }
    if (front_end == 0) 
    {
        bool invalid_command = true;
        bool need_loop = false;
        while (1)
        {
            while(invalid_command) {
                printf("JogoDaVelha> ");
                fgets(user_input, 64, stdin);
                user_input[strlen(user_input) - 1] = '\0'; 
                printf("Front-end Input: %s\n", user_input);
                strncpy(user_input_copy, user_input, strlen(user_input));
                user_input_copy[strlen(user_input)] = '\0';
                command = strtok(user_input_copy, " ");
                client_message[strlen(client_message) - 1] = '\0';
            /*  NEW    ___________________________________________________________________*/
                if (!strncmp(command, "new", 3)) 
                {
                    invalid_command = false;
                }
            /*  PASS    __________________________________________________________________*/
                else if (!strncmp(command, "pass", 4)) 
                { 
                    invalid_command = false;
                }
            /*  IN    ____________________________________________________________________*/
                else if (!strncmp(command, "in", 2)) 
                { 
                    invalid_command = false;
                }
            /*  HALLOFFAME    ____________________________________________________________*/
                else if (!strncmp(command, "halloffame", 10)) 
                {
                    printf("\n*** Hall of Fame ***\n\n");
                    invalid_command = false;
                    need_loop = true; 
                }
            /*  L    _____________________________________________________________________*/
                else if (!strncmp(command, "l", 1)) 
                {
                    printf("\nUsuários Online\n");
                    printf("(usuário) | (jogando)\n");
                    invalid_command = false;
                    need_loop = true; 
                }
            /*  CALL    __________________________________________________________________*/ 
                else if (!strncmp(command, "call", 4)) 
                { 
                    invalid_command = false;
                } 
            /*  PLAY    __________________________________________________________________*/
                else if (!strncmp(command, "play", 4)) 
                { 
                    invalid_command = false;
                }
            /*  DELAY    _________________________________________________________________*/ 
                else if (!strncmp(command, "delay", 5)) 
                { 
                    invalid_command = false;
                }
            /*  OVER    __________________________________________________________________*/ 
                else if (!strncmp(command, "over", 4)) 
                { 
                    invalid_command = false;
                }
            /*  OUT    ___________________________________________________________________*/
                else if (!strncmp(command, "out", 3)) 
                { 
                    invalid_command = false;
                }
            /*  BYE    ___________________________________________________________________*/
                else if (!strncmp(command, "bye", 3)) 
                { 
                    invalid_command = false;
                    write(front_end_pipe, (void *) user_input, (size_t) strlen(user_input));
                    printf ("Ending the game... Good bye!\n");
                    sleep (2);
                    return 0;
                } 
            /*  ELSE    __________________________________________________________________*/
                else
                {
                    printf("    Comando inválido ...Digite novamente!\n");
                }
            }
            write(front_end_pipe, (void *) user_input, (size_t) strlen(user_input));
            if (need_loop)
            {
                read(back_end_pipe, (void *) server_message, (size_t) sizeof(server_message));
                printf("    %s\n", server_message);
                if (!strncmp(server_message, NACK_not_logged, sizeof(NACK_not_logged))) 
                {
                    need_loop = false;
                    continue;
                }
                while (strncmp(server_message, ACK_hallofame, sizeof(ACK_hallofame)) &&
                       strncmp(server_message, ACK_online_l, sizeof(ACK_online_l)))
                {
                    read(back_end_pipe, (void *) server_message, (size_t) sizeof(server_message));
                    printf("    %s\n", server_message);
                } 
                need_loop = false;
            }
            else
            {
                read(back_end_pipe, (void *) server_message, (size_t) sizeof(server_message));
                printf("Front-end Output: %s\n", server_message);
            }
            memset((void *) user_input, 0, sizeof(user_input));
            memset((void *) server_message, 0, sizeof(server_message));
            invalid_command = true;
            need_loop = false;
        }
        exit(EXIT_SUCCESS);
    }
    return front_end;
}

int Connect_Procedure(bool is_udp, int client_sockfd, struct sockaddr_in * serv_addr)
{
    socklen_t len;
    ssize_t n_bytes;

    /*Protocolos: */
    unsigned char CONNECT;
    unsigned char ACK_NACK;
    uint16_t CHANGE_PORT;

    if (is_udp)
    {
        if ((client_sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1 )
        {
            printf("Error: socket not created\n");
            return -1;
        }
        if (sendto(client_sockfd, (const char *) &CONNECT, sizeof(CONNECT), 0, 
                    (const struct sockaddr *) serv_addr, sizeof(struct sockaddr_in)) == -1)
        {
            printf("Error: sendto failed!\n");
            return -1;
        }
        printf("    enviou CONNECT %d\n", CONNECT);
        len = sizeof(struct sockaddr_in);
        if (recvfrom(client_sockfd, &ACK_NACK, sizeof(ACK_NACK), 0, 
                        (struct sockaddr *) serv_addr, &len) == -1)
        {
            printf("Error: recvfrom failed!\n");
            return -1;
        }
        if (ACK_NACK == 1)
        {
            printf("    Alcançou o servidor.\n");
        }
        else
        {
            return -1;
        }
        if (recvfrom(client_sockfd, (void*) &CHANGE_PORT, sizeof(CHANGE_PORT), 0, 
                        (struct sockaddr *) serv_addr, &len) == -1)
        {
            printf("Error: recvfrom failed!\n");
            return -1;
        }
        if (CHANGE_PORT != 0)
        {
            close(client_sockfd);
            printf("Changing to aux port: %d\n", ntohs(CHANGE_PORT));
            bzero(serv_addr, sizeof(struct sockaddr_in));
            (*serv_addr).sin_family = AF_INET;
            (*serv_addr).sin_port = CHANGE_PORT;
            (*serv_addr).sin_addr.s_addr = inet_addr("192.168.15.15");
            if ((client_sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1 )
            {
                printf("Error: socket not created\n");
                return -1;
            }
        }
        return client_sockfd;
    }
    /*  TCP    _______________________________________________________________*/
    else
    {
        if ((client_sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1 )
        {
            printf("Error: socket not created\n");
            return -1;
        }
        if ((connect(client_sockfd, (const struct sockaddr *) serv_addr,
                        sizeof(struct sockaddr_in))) == -1)
        {
            printf("Error: connect failed\n");
            return -1;
        }
        printf("Connect success.\n");
        if (send(client_sockfd, (const char *) &CONNECT, sizeof(CONNECT), 0) == -1)
        {
            printf("Error: send failed!\n");
            return -1;
        }
        printf("    enviou CONNECT %d\n", CONNECT);
        if (recv(client_sockfd, &ACK_NACK, sizeof(ACK_NACK), 0) == -1)
        {
            printf("Error: recv failed!\n");
            return -1;
        }
        if (ACK_NACK == 1)
        {
            printf("    Alcançou o servidor.\n");
            return client_sockfd;
        }
    }
}
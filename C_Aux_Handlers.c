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
#include "Protocol.h"



pid_t listener_process(int client_sockfd, bool is_udp,
                        struct sockaddr_in * serv_addr,
                        int listener_pipe, bool DEBUG) 
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
                if(DEBUG) printf("\n[Listener] Listener não recebeu nenhuma conexão do servidor\n");
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
                if(DEBUG) printf("[Listener] Listener recebeu do socket: %s\n", server_message);
                if(DEBUG) printf("[Listener]    n_bytes: %d\n", (int) n_bytes);
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
                        int sender_pipe, bool DEBUG) 
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
                read(sender_pipe, (void *) client_message, sizeof(client_message));
                if(DEBUG) printf("[Sender] Sender recebeu do pipe: %s\n", client_message);
                if (strlen(client_message))
                {
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
                    if(DEBUG) printf("[Sender]    .... Sender enviou mensagem para Servidor\n");
                    if(DEBUG) printf("[Sender]    n_bytes: %d\n", (int) n_bytes);
                }
            }
        }
        exit(EXIT_SUCCESS);
    }
    return sender;
}

pid_t front_end_process(int back_end_pipe, int front_end_pipe, bool DEBUG) 
{
    pid_t  front_end;
    int n_bytes, len, ret; 
    char user_input[64], user_input_copy[64], server_message[64], client_message[64];
    char other_player_name[64];
    char * command, * player2;
    bool logged = false;

    if ((front_end = fork()) == -1)
    {
        printf("Erro: fork front_end failed\n");
        exit(EXIT_FAILURE);
    }
    if (front_end == 0) 
    {
        if(DEBUG) printf("[Front-end] front_end created\n");
        bool invalid_command = true;
        bool need_loop = false;
        while (1)
        {
            while(invalid_command) {
                printf("JogoDaVelha> ");
                fgets(user_input, 64, stdin);
                if (!strncmp(user_input, "\n", 2))
                {
                    continue;
                }
                user_input[strlen(user_input) - 1] = '\0'; 
                if(DEBUG) printf("[Front-end] Input: %s\n", user_input);
                strncpy(user_input_copy, user_input, strlen(user_input));
                user_input_copy[strlen(user_input)] = '\0';
                command = strtok(user_input_copy, " ");
                client_message[strlen(client_message) - 1] = '\0';
            /*  NEW    ___________________________________________________________________*/
                if (!strncmp(command, "new", 4)) 
                {
                    invalid_command = false;
                }
            /*  PASS    __________________________________________________________________*/
                else if (!strncmp(command, "pass", 5)) 
                { 
                    invalid_command = false;
                }
            /*  IN    ____________________________________________________________________*/
                else if (!strncmp(command, "in", 3)) 
                { 
                    invalid_command = false;
                }
            /*  HALLOFFAME    ____________________________________________________________*/
                else if (!strncmp(command, "halloffame", 11)) 
                {
                    printf("\n*** Hall of Fame ***\n\n");
                    invalid_command = false;
                    need_loop = true; 
                }
            /*  L    _____________________________________________________________________*/
                else if (!strncmp(command, "l", 2)) 
                {
                    printf("\nUsuários Online:\n");
                    printf("(usuário) | (jogando)\n");
                    invalid_command = false;
                    need_loop = true; 
                }
            /*  CALL    __________________________________________________________________*/ 
                else if (!strncmp(command, "call", 5)) 
                { 
                    invalid_command = false;
                    player2 = strtok(NULL, " ");
                    strncpy(other_player_name, player2, strlen(player2));
                    other_player_name[strlen(player2)] = '\0';
                    if(DEBUG) printf ("[Front-end] other_player_name: %s\n", other_player_name);
                } 
            /*  PLAY    __________________________________________________________________*/
                else if (!strncmp(command, "play", 5)) 
                { 
                    invalid_command = false;
                }
            /*  DELAY    _________________________________________________________________*/ 
                else if (!strncmp(command, "delay", 6)) 
                { 
                    invalid_command = false;
                }
            /*  OVER    __________________________________________________________________*/ 
                else if (!strncmp(command, "over", 5)) 
                { 
                    invalid_command = false;
                }
            /*  OUT    ___________________________________________________________________*/
                else if (!strncmp(command, "out", 4)) 
                { 
                    invalid_command = false;
                }
            /*  BYE    ___________________________________________________________________*/
                else if (!strncmp(command, "bye", 4)) 
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
                while (strncmp(server_message, ACK_hallofame, sizeof(ACK_hallofame)) &&
                       strncmp(server_message, ACK_online_l, sizeof(ACK_online_l))  &&
                       strncmp(server_message, NACK_not_logged, sizeof(NACK_not_logged)))
                {
                    read(back_end_pipe, (void *) server_message, (size_t) sizeof(server_message));
                    if (!strncmp(server_message, ACK_hallofame, sizeof(ACK_hallofame)))
                    {
                        printf("\n%s\n", server_message); //End of hall of fame
                    }
                    else if (!strncmp(server_message, ACK_online_l, sizeof(ACK_online_l)))
                    {
                        printf("                        %s\n", server_message); //End of list
                    }
                    else
                    {
                        printf("    %s\n", server_message); //Normal Case
                    } 
                } 
                need_loop = false;
            }
            else
            {
                read(back_end_pipe, (void *) server_message, (size_t) sizeof(server_message));
                printf("    %s\n", server_message);
            }
            /*
                resposta servidor
            */
            memset((void *) user_input, 0, sizeof(user_input));
            memset((void *) server_message, 0, sizeof(server_message));
            invalid_command = true;
            need_loop = false;
        }
        exit(EXIT_SUCCESS);
    }
    return front_end;
}

int Connect_Procedure(bool is_udp, int client_sockfd, 
                        struct sockaddr_in * serv_addr, bool DEBUG)
{
    socklen_t len;
    ssize_t n_bytes;

    /*Protocolos: */
    unsigned char CONNECT;
    unsigned char ACK_NACK;
    uint16_t CHANGE_PORT;

    CONNECT = 1;

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
        if(DEBUG) printf("[Connect Procedure]    enviou CONNECT %d\n", CONNECT);
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
            if(DEBUG) printf("[Connect Procedure] Changing to aux port: %d\n", ntohs(CHANGE_PORT));
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
        printf("Conexão estabelecida.\n");
        if (send(client_sockfd, (const char *) &CONNECT, sizeof(CONNECT), 0) == -1)
        {
            printf("Error: send failed!\n");
            return -1;
        }
        if(DEBUG) printf("[Connect Procedure] enviou CONNECT %d\n", CONNECT);
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
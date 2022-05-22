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

#include "Client_Handler.h"
#include "DB_Manag_Sys.h"


/*****************************************************************************************************/
/*    UDP CLIENT HANDLER                                                                             */
/*****************************************************************************************************/
void udp_client_handler(int pipe_read, int pipe_write, uint16_t port) 
{
    char ACK_new_user[21]  = "...new user created!";
    char NACK_new_user[19] = "...new user failed";
    char ACK_in_user[11]   = "...logged!";
    char NACK_in_user[47]  = "...not logged, username or password incorrect ";
    char ACK_out_user[15]  = "...logged out!";
    char NACK_out_user[23] = "...error: still logged";

    int udp_fd, tcp_fd;
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

    memset((void *)&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    len = sizeof(addr);

    
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

    if ((listener = fork()) == -1)
    {
        printf("Erro: fork listener from udp_client_handler failed\n");
        exit(EXIT_FAILURE);
    }
    if (listener == 0) // Is listener
    {
        close(pipe_read);
        close(pipe_write);
    
        if ((n_bytes = recvfrom(udp_fd, (void *) client_message, sizeof(client_message), 0,
                (struct sockaddr *) &addr, (socklen_t *) &len) == -1))
        {
            printf("Erro: recvfrom from udp_client_handler failed\n");
            exit(EXIT_FAILURE);
        }
        
        port = addr.sin_port;
        write(list_to_send_pipe[1],(void *) &port, sizeof(port));
        printf("Listener: Porta do cliente UDP: %d\n", ntohs(addr.sin_port));
        printf("Listener recebeu do socket: %s\n", client_message);
        write(listener_pipe[1], (void *) client_message, (size_t) sizeof(client_message));
        
        while (1)
        {
            // fazer o poll enviar um ping caso timeout
            memset((void *)client_message, 0, sizeof(client_message));
            if ((n_bytes = recvfrom(udp_fd, (void *) client_message, sizeof(client_message), 0,
                (struct sockaddr *) &addr, (socklen_t *) &len) == -1))
            {
                printf("Erro: recvfrom from udp_client_handler failed\n");
                exit(EXIT_FAILURE);
            }
            printf("Listener recebeu do socket: %s\n", client_message);
            write(listener_pipe[1], (void *) client_message, (size_t) sizeof(client_message));
        }
        exit(EXIT_SUCCESS);
    }

    if ((sender = fork()) == -1)
    {
        printf("Erro: fork sender from udp_client_handler failed\n");
        exit(EXIT_FAILURE);
    }
    if (sender == 0) 
    {
        close(pipe_read);
        close(pipe_write);
        read(list_to_send_pipe[0], (void *) &port, sizeof(port));
        addr.sin_port = port;
        printf("Sender: Porta do cliente UDP: %d\n", ntohs(addr.sin_port));
        while (1)
        {
            read(sender_pipe[0], (void *) server_message, (size_t) sizeof(server_message));
            printf("Sender recebeu do pipe: %s\n", server_message);
            if ((n_bytes = sendto(udp_fd, (void *) server_message, strlen(server_message), 0,
                (const struct sockaddr *) &addr, (socklen_t ) sizeof(addr)) == -1))
            {
                printf("Erro: sendto from udp_client_handler failed\n");
                exit(EXIT_FAILURE);
            }
            printf("   .... Sender enviou mensagem para Cliente\n");
            memset((void *)server_message, 0, sizeof(server_message));
        }
        exit(EXIT_SUCCESS);
    }
    else // Is Processor 
    {
        close(udp_fd);
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
                printf("Error: poll from handler failed");
                exit(EXIT_FAILURE);
            }
            else if ((poll_fd[0].revents == POLLIN) && (poll_fd[0].fd == listener_pipe[0]))
            {
                read(listener_pipe[0], (void *) client_message, (size_t) sizeof(client_message));
                printf("Processador recebeu do listener: %s\n", client_message);
                /*
                    Processa mensagem do listener, return client_message;
                */
                write(pipe_write, (void *) client_message, (size_t) sizeof(client_message));
                memset(client_message, 0, sizeof(client_message));
            }
            else if ((poll_fd[1].revents == POLLIN) && (poll_fd[1].fd == pipe_read))
            {
                read(pipe_read, (void *) server_message, (size_t) sizeof(server_message));
                printf("Processador  recebeu do main: %s\n", server_message);
                /*
                    Processa mensagem do outro processo do servidor, return server_message;
                */
                write(sender_pipe[1], (void *) server_message, (size_t) sizeof(server_message));
                memset(server_message, 0, sizeof(server_message));
            }
        }
    }  
}


/*****************************************************************************************************/
/*    TCP CLIENT HANDLER                                                                             */
/*****************************************************************************************************/
void tcp_client_handler(int pipe_read, int pipe_write, int tcp_fd, struct sockaddr_in * addr) 
{
    char ACK_new_user[21]  = "...new user created!";
    char NACK_new_user[19] = "...new user failed";
    char ACK_in_user[11]   = "...logged!";
    char NACK_in_user[47]  = "...not logged, username or password incorrect ";
    char ACK_out_user[15]  = "...logged out!";
    char NACK_out_user[23] = "...error: still logged";
    
    ssize_t n_bytes;
    socklen_t len;
    pid_t listener;
    pid_t sender;

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
    
        if ((n_bytes = recv(tcp_fd, (void *) client_message, sizeof(client_message), 0) == -1))
        {
            printf("Erro: recv from listener tcp_client_handler failed\n");
            exit(EXIT_FAILURE);
        }
        
        // port = addr.sin_port;
        // write(list_to_send_pipe[1],(void *) &port, sizeof(port));
        // printf("Listener: Porta do cliente UDP: %d\n", ntohs(addr.sin_port));
        printf("Listener recebeu do socket: %s\n", client_message);
        write(listener_pipe[1], (void *) client_message, (size_t) sizeof(client_message));
        n_bytes = 1;
        while (n_bytes)
        {
            // fazer o poll enviar um ping caso timeout
            memset((void *)client_message, 0, sizeof(client_message));
            n_bytes = recv(tcp_fd, (void *) client_message, sizeof(client_message), 0);
            if (n_bytes == -1)
            {
                printf("Erro: recvfrom from listener udp_client_handler failed\n");
                exit(EXIT_FAILURE);
            }
            printf("Listener recebeu do socket: %s\n", client_message);
            write(listener_pipe[1], (void *) client_message, (size_t) sizeof(client_message));
            printf("    n_bytes: %d\n", (int) n_bytes);
        }
        exit(EXIT_SUCCESS);
    }

    if ((sender = fork()) == -1)
    {
        printf("Erro: fork sender from udp_client_handler failed\n");
        exit(EXIT_FAILURE);
    }
    if (sender == 0) 
    {
        close(pipe_read);
        close(pipe_write);
        // read(list_to_send_pipe[0], (void *) &port, sizeof(port));
        // addr.sin_port = port;
        // printf("Sender: Porta do cliente UDP: %d\n", ntohs(addr.sin_port));
        n_bytes = 1;
        while (1)
        {
            read(sender_pipe[0], (void *) server_message, (size_t) sizeof(server_message));
            printf("Sender recebeu do pipe: %s\n", server_message);
            n_bytes = send(tcp_fd, (void *) server_message, strlen(server_message), 0);
            if (n_bytes == -1)
            {
                printf("Erro: send from udp_client_handler failed\n");
                exit(EXIT_FAILURE);
            }
            printf("   .... Sender enviou mensagem para Cliente\n");
            memset((void *) server_message, 0, sizeof(server_message));
            printf("    n_bytes: %d\n", (int) n_bytes);
        }
        exit(EXIT_SUCCESS);
    }
    else // Is Processor 
    {
        close(tcp_fd);
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
                printf("Error: poll from handler failed");
                exit(EXIT_FAILURE);
            }
            else if ((poll_fd[0].revents == POLLIN) && (poll_fd[0].fd == listener_pipe[0]))
            {
                read(listener_pipe[0], (void *) client_message, (size_t) sizeof(client_message));
                printf("Processador recebeu do listener: %s\n", client_message);
                /*
                    Processa mensagem do listener, return client_message;
                */
                write(pipe_write, (void *) client_message, (size_t) sizeof(client_message));
                memset(client_message, 0, sizeof(client_message));
            }
            else if ((poll_fd[1].revents == POLLIN) && (poll_fd[1].fd == pipe_read))
            {
                read(pipe_read, (void *) server_message, (size_t) sizeof(server_message));
                printf("Processador  recebeu do main: %s\n", server_message);
                /*
                    Processa mensagem do outro processo do servidor, return server_message;
                */
                write(sender_pipe[1], (void *) server_message, (size_t) sizeof(server_message));
                memset(server_message, 0, sizeof(server_message));
            }
        }
    }  
}

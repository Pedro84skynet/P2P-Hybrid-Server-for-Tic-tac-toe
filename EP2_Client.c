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

#include "Hash_Game.h"

struct client_info {
    char ip[16];
    uint16_t port;
    uint16_t P2P_port;
    bool main;
};


// Para o Cliente: 
//     TCP Client: socket[] -> connect[] -> send[] || receive[]
//     UDP Client: socket[] -> sendto[]
// Para o P2P: 
//     TCP Server: socket[] -> bind[] -> listen[] -> accept[] 

int main(int argc, char ** argv) 
{
    socklen_t len;
    ssize_t nbytes;
    int client_sockfd, listen_fd, player_fd;
    struct sockaddr_in serv_addr, player2_addr;
    uint16_t port;

    struct client_info other_player;

    /*Protocolos: */
    unsigned char CONNECT;
    unsigned char ACK_NACK;
    uint16_t CHANGE_PORT;

    CONNECT = 1;

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(atoi(argv[1]));
    serv_addr.sin_addr.s_addr = inet_addr("192.168.15.15");

    char user_input[64], user_input_copy[64];
    //char * user_input_copy;
    char * command;
    char * username;
    char * password;
    bool logged = false;

    unsigned char * hashtable;
    unsigned char game_end;
    game_end = 0;
    int line, column;
    int tie = 5;
    bool game_on = false;
    bool is_player1 = false;

    bool is_udp;

    if (!strncmp("UDP", argv[2], 3) || 
        !strncmp("udp", argv[2], 3) || 
        !strncmp("Udp", argv[2], 3)) is_udp = true;
    else is_udp = false;
   

    if (is_udp)
    {
        if ((client_sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1 )
        {
            printf("Error: socket not created\n");
            exit(EXIT_FAILURE);
        }
        if (sendto(client_sockfd, (const char *) &CONNECT, sizeof(CONNECT), 0, 
                    (const struct sockaddr *) &serv_addr, sizeof(serv_addr)) == -1)
        {
            printf("Error: sendto failed!\n");
            exit(EXIT_FAILURE);
        }
        printf("    enviou CONNECT %d\n", CONNECT);
        len = sizeof(serv_addr);
        if (recvfrom(client_sockfd, &ACK_NACK, sizeof(ACK_NACK), 0, 
                        (struct sockaddr *) &serv_addr, &len) == -1)
        {
            printf("Error: recvfrom failed!\n");
            exit(EXIT_FAILURE);
        }
        if (ACK_NACK == 1)
        {
            printf("    Alcançou o servidor.\n");
        }
        if (recvfrom(client_sockfd, (void*) &CHANGE_PORT, sizeof(CHANGE_PORT), 0, 
                        (struct sockaddr *) &serv_addr, &len) == -1)
        {
            printf("Error: recvfrom failed!\n");
            exit(EXIT_FAILURE);
        }
        if (CHANGE_PORT != 0)
        {
            close(client_sockfd);
            printf("Changing to aux port: %d\n", ntohs(CHANGE_PORT));
            bzero(&serv_addr, sizeof(serv_addr));
            serv_addr.sin_family = AF_INET;
            serv_addr.sin_port = CHANGE_PORT;
            serv_addr.sin_addr.s_addr = inet_addr("192.168.15.15");
            if ((client_sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1 )
            {
                printf("Error: socket not created\n");
                exit(EXIT_FAILURE);
            }
        }
    }
    else
    {
        if ((client_sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1 )
        {
            printf("Error: socket not created\n");
            exit(EXIT_FAILURE);
        }
        if ((connect(client_sockfd, (const struct sockaddr *) &serv_addr,
                        sizeof(serv_addr))) == -1)
        {
            printf("Error: connect failed\n");
            exit(EXIT_FAILURE);
        }
        if (send(client_sockfd, (const char *) &CONNECT, sizeof(CONNECT), 0) == -1)
        {
            printf("Error: send failed!\n");
            exit(EXIT_FAILURE);
        }
        printf("    enviou CONNECT %d\n", CONNECT);
        if (recv(client_sockfd, &ACK_NACK, sizeof(ACK_NACK), 0) == -1)
        {
            printf("Error: recv failed!\n");
            exit(EXIT_FAILURE);
        }
        if (ACK_NACK == 1)
        {
            printf("    Alcançou o servidor.\n");
        }
    }

    struct client_info player1;

    ssize_t n_bytes;
    pid_t sender, listener, front_end;

    int listener_pipe[2]; // listener_pipe[0] <- Read; listener_pipe[1] <- Write;
    pipe(listener_pipe);
    int sender_pipe[2]; // sender_pipe[0] <- Read; sender_pipe[1] <- Write;
    pipe(sender_pipe);
    int front_end_pipe[2]; // front_end_pipe[0] <- Read; front_end_pipe[1] <- Write;
    pipe(front_end_pipe);
    int back_end_pipe[2]; // back_end_pipe[0] <- Read; back_end_pipe[1] <- Write;
    pipe(back_end_pipe);


    unsigned char ping[1] = {1}; 
    unsigned char client_message[64]; 
    unsigned char server_message[64]; 
    unsigned char processed_message[64]; 

    struct pollfd poll_fd[2];
    int ret, t_out;
    int timeout = 3*60*1000; //milésimos

    len = sizeof(serv_addr);

    if ((listener = fork()) == -1)
    {
        printf("Erro: fork listener failed\n");
        exit(EXIT_FAILURE);
    }
    if (listener == 0) // Is listener
    {
        close(front_end_pipe[0]);
        close(front_end_pipe[1]);
        close(back_end_pipe[0]);
        close(back_end_pipe[1]);

        n_bytes = 1;
        while (n_bytes) 
        {
            // fazer o poll e enviar um ping caso timeout

            if (is_udp)
            {
                n_bytes = recvfrom(client_sockfd, (void *) server_message, sizeof(server_message), 0,
                    (struct sockaddr *) &serv_addr, (socklen_t *) &len);
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
            }
            
            printf("Listener recebeu do socket: %s\n", server_message);
            write(listener_pipe[1], (void *) server_message, (size_t) sizeof(server_message));
            memset((void *)server_message, 0, sizeof(server_message));
            printf("    n_bytes: %d\n", (int) n_bytes);
        }
        exit(EXIT_SUCCESS);
    }
    if ((sender = fork()) == -1)
    {
        printf("Erro: fork sender failed\n");
        exit(EXIT_FAILURE);
    }
    if (sender == 0) 
    {
        close(front_end_pipe[0]);
        close(front_end_pipe[1]);
        close(back_end_pipe[0]);
        close(back_end_pipe[1]); 
        
        n_bytes = 1;
        while (n_bytes)
        {
            read(sender_pipe[0], (void *) client_message, (size_t) sizeof(client_message));
            printf("Sender recebeu do pipe: %s\n", client_message);

            if (is_udp)
            {
                n_bytes = sendto(client_sockfd, (void *) client_message, strlen(client_message), 0,
                    (struct sockaddr *) &serv_addr, (socklen_t ) sizeof(serv_addr));
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
            memset((void *)client_message, 0, sizeof(client_message));
            printf("    n_bytes: %d\n", (int) n_bytes);
        }
        exit(EXIT_SUCCESS);
    }
    if ((front_end = fork()) == -1)
    {
        printf("Erro: fork front_end failed\n");
        exit(EXIT_FAILURE);
    }
    if (front_end == 0) // Is front_end
    {
        close(listener_pipe[0]);
        close(listener_pipe[1]);
        close(sender_pipe[0]);
        close(sender_pipe[1]);
        close(client_sockfd);
        while (1)
        {
            fgets(client_message, 64, stdin);
            client_message[strlen(client_message) - 1] = '\0';
            printf("Front-end Input: %s\n", client_message);
            /*
                    Aquarda e recebe input do client, return client_message;
            */
            write(front_end_pipe[1], (void *) client_message, (size_t) sizeof(client_message));
            read(back_end_pipe[0], (void *) server_message, (size_t) sizeof(server_message));
            /*
                    Recebe resposta do Main e retorna visualização para o cliente, return server_message;
            */
            printf("Front-end Output: %s\n", server_message);
            memset((void *) client_message, 0, sizeof(client_message));
            memset((void *) server_message, 0, sizeof(server_message));
        }
        exit(EXIT_SUCCESS);
    }
    else // Is Main 
    {
        while(1) 
        {
            poll_fd[0].fd = front_end_pipe[0];
            poll_fd[0].events = POLLIN;
            poll_fd[1].fd = listener_pipe[0];
            poll_fd[1].events = POLLIN;

            memset((void *)client_message, 0, 64);
            ret = poll(poll_fd, 2,  0);
            if (ret == -1)
            {
                printf("Error: poll from handler failed");
                exit(EXIT_FAILURE);
            }
            if ((poll_fd[0].revents == POLLIN) && (poll_fd[0].fd == front_end_pipe[0]))
            {
                read(front_end_pipe[0], (void *) client_message, (size_t) sizeof(client_message));
                printf("Main recebeu do front_end: %s\n", client_message);
                /*
                    Processa mensagem do front_end_pipe e envia para sender, return client_message;
                */
                write(sender_pipe[1], (void *) client_message, (size_t) sizeof(client_message));
                usleep(50000);
            }
            if ((poll_fd[1].revents == POLLIN) && (poll_fd[1].fd == listener_pipe[0]))
            {
                read(listener_pipe[0], (void *) server_message, (size_t) sizeof(server_message));
                printf("Main recebeu do main_pipe: %s\n", server_message);
                /*
                    Processa mensagem do listener, return processed_message;
                */
                strncpy(processed_message, server_message, 64);
                write(back_end_pipe[1], (void *) processed_message, (size_t) sizeof(processed_message));
                usleep(50000);
            }
        }
    }
} 

 
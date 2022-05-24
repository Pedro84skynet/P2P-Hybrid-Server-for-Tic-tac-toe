/******************************************************************************
 *  Compilation:  (Use make)
 *  Execution:    ./EP2_Client port_number protocol
 *
 *  - port_number: port number used to connect in EP2_Servidor.
 *  - protocol   : "UDP" or "TCP"
 *
 *  DESCRIPTION
 *
 *  A brief description about what is implemented.
 *
 *  PROJECT DECISIONS OR UNFINISHED TASKS (?)
 *
 *  List them bellow
 *
 *  - Don't forget to change var "serv_addr.sin_addr.s_addr" to your IP
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


// Para o Cliente: 
//     TCP Client: socket[] -> connect[] -> send[] || receive[]
//     UDP Client: socket[] -> sendto[]
// Para o P2P: 
//     TCP Server: socket[] -> bind[] -> listen[] -> accept[] 

/*****************************************************************************************************/
/*    MAIN                                                                                           */
/*****************************************************************************************************/
int main(int argc, char ** argv) 
{

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
    char Server_down[32]         = "...lost connection with server.";

    // Message error
    if (argc < 2) {
        printf("usage: ./EP2_Client port_number protocol\n");
        printf("examples with port = 8000:\n");
        printf("    Protocol TCP:         ./EP2_Client 8000 TCP\n");
        printf("    Protocol UDP:         ./EP2_Client 8000 UDP\n");
        exit(0);
    }

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
    //serv_addr.sin_addr.s_addr = inet_addr("192.168.15.138");
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


    unsigned char client_message[64]; 
    unsigned char server_message[64]; 
    unsigned char processed_message[64]; 

    struct pollfd poll_fd[2];
    int ret;


    /**************************************************************************

     Checks the protocol and if client connected to server

    ***************************************************************************/
    if (!strncmp("UDP", argv[2], 3) || 
        !strncmp("udp", argv[2], 3) || 
        !strncmp("Udp", argv[2], 3)) 
    {
        is_udp = true;
    }
    else if (!strncmp("TCP", argv[2], 3) || 
             !strncmp("tcp", argv[2], 3) || 
             !strncmp("Tcp", argv[2], 3)) 
    {
        is_udp = false;
    }
    else
    {
        printf("Protocolo não utilizado.\n");
        exit(EXIT_FAILURE);
    } 
    /*  UDP    _______________________________________________________________*/
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
    /*  TCP    _______________________________________________________________*/
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

    

    listener = listener_process(client_sockfd, is_udp, &serv_addr, listener_pipe[1]);
    sender = sender_process(client_sockfd, is_udp, &serv_addr, sender_pipe[0]);
    front_end = front_end_process(back_end_pipe[0], front_end_pipe[1]); 
    if (listener == 0 || sender == 0 || front_end == 0)
    {
        return 0;
    }
    
    while(1) // Is Main
    {
        poll_fd[0].fd = front_end_pipe[0];
        poll_fd[0].events = POLLIN;
        poll_fd[1].fd = listener_pipe[0];
        poll_fd[1].events = POLLIN;

        memset((void *)client_message, 0, 64);
        ret = poll(poll_fd, 2,  -1);
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
            if (!strncmp (client_message, "bye", 3)) {
                printf("Bye command ...exiting!\n");
                write (sender_pipe[1], (void *) client_message, (size_t) sizeof(client_message));
                sleep (1);
                kill (sender, SIGKILL);
                kill (listener, SIGKILL);
                // front end terminates itself (return 0)
                return 0;
            }
            else {
                write(sender_pipe[1], (void *) client_message, (size_t) sizeof(client_message));
                usleep(50000);
            }
        }
        if ((poll_fd[1].revents == POLLIN) && (poll_fd[1].fd == listener_pipe[0]))
        {
            read(listener_pipe[0], (void *) server_message, (size_t) sizeof(server_message));
            printf("Main recebeu do main_pipe: %s\n", server_message);
            /*
                Processa mensagem do listener, return processed_message;
            */
            if (!strncmp (server_message, Server_down, sizeof(Server_down)))
            {
                printf("\n%s\n", server_message);
                write (back_end_pipe[1], (void *) server_message, (size_t) sizeof(server_message));
                kill (sender, SIGKILL);
                kill (listener, SIGKILL);
                sleep (2);
                kill (front_end, SIGKILL);
                return 0; 
            }
            strncpy(processed_message, server_message, 64);
            write(back_end_pipe[1], (void *) processed_message, (size_t) sizeof(processed_message));
            usleep(50000);
        }
    }
} 

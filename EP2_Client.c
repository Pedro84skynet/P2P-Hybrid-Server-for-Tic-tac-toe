/******************************************************************************
 *  Compilation:  (Use make)
 *  Execution:    ./EP2_Client port_number protocol
 *
 *  - port_number: port number used to connect in EP2_Servidor.
 *  - protocol   : "UDP" or "TCP"
 *
 *  DESCRIPTION
 *
 *  A client to tic-tac-toe local network game.
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
#include "Protocol.h"


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

    // Error Message 
    if (argc < 2) {
        printf("usage: ./EP2_Client port_number protocol\n");
        printf("examples with port = 8000:\n");
        printf("    Protocol TCP:         ./EP2_Client 8000 TCP\n");
        printf("    Protocol UDP:         ./EP2_Client 8000 UDP\n");
        exit(0);
    }

    socklen_t len;
    ssize_t n_bytes;
    int client_sockfd, listen_fd, player_fd;
    uint16_t port;



    /*
        Basic Protocolos. 
    */
    unsigned char CONNECT;
    unsigned char ACK_NACK;
    uint16_t CHANGE_PORT;

    CONNECT = 1;

    struct sockaddr_in serv_addr;
    memset((void *)&serv_addr, 0,  sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(atoi(argv[1]));
    //serv_addr.sin_addr.s_addr = inet_addr("192.168.15.138");
    serv_addr.sin_addr.s_addr = inet_addr("192.168.15.15");

    struct sockaddr_in own_addr;
    memset((void *)&own_addr, 0, sizeof(own_addr));
    own_addr.sin_family = AF_INET;
    own_addr.sin_port = htons(atoi(argv[1]) +1);
    own_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    char user_input[64], user_input_copy[64];
    char * command;
    char * username;
    char * othername;
    char * other_ip;
    char * password;
    char accept_call;
    bool logged = false;
    int try_c;

    unsigned char * hashtable;
    unsigned char game_end;
    game_end = 0;
    int line, column;
    int tie = 5;
    bool game_on = false;
    bool is_player1 = false;

    bool is_udp;

    pid_t sender, listener, front_end;

    /* 
        Pipes for communication between processes
    */
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

    /*
        Checks the protocol and if client connected to server
    */
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

    client_sockfd = Connect_Procedure(is_udp, client_sockfd, &serv_addr);
    if (client_sockfd == -1)
    {
        printf("Error: connection failed!\n");
        exit(EXIT_FAILURE);
    }

    /* 
        Initialize auxiliars processes
    */
    listener  = listener_process(client_sockfd, is_udp, &serv_addr, listener_pipe[1]);
    sender    = sender_process(client_sockfd, is_udp, &serv_addr, sender_pipe[0]);
    front_end = front_end_process(back_end_pipe[0], front_end_pipe[1]); 
    if (listener == 0 || sender == 0 || front_end == 0)
    {
        return 0;
    }

    /* 
        Main process
    */
    while(1) 
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
        /*
            Processa mensagem do front_end_pipe e envia para sender, return client_message;
        */
        if ((poll_fd[0].revents == POLLIN) && (poll_fd[0].fd == front_end_pipe[0]))
        {
            read(front_end_pipe[0], (void *) client_message, (size_t) sizeof(client_message));
            printf("Main recebeu do front_end: %s\n", client_message);
            
            if (!strncmp (client_message, "bye", 3)) {
                printf("Bye command ...exiting!\n");
                write(sender_pipe[1], (void *) client_message, (size_t) sizeof(client_message));
                sleep (1);
                kill(sender, SIGKILL);
                kill(listener, SIGKILL);
                // front end terminates itself (return 0)
                return 0;
            }
            else {
                write(sender_pipe[1], (void *) client_message, (size_t) sizeof(client_message));
                usleep(50000);
            }
        }
        /*
            Processa mensagem do listener, return processed_message;
        */
        if ((poll_fd[1].revents == POLLIN) && (poll_fd[1].fd == listener_pipe[0]))
        {
            read(listener_pipe[0], (void *) server_message, (size_t) sizeof(server_message));
            printf("Main recebeu do main_pipe: %s\n", server_message);
        
        /*  Specials Cases*/
        /*  CALL  */
            if (!strncmp (server_message, "call", 4))
            {
                memset((void *) processed_message, 0, sizeof(processed_message));
                strncpy(processed_message, server_message, strlen(server_message));
                processed_message[strlen(server_message)] = '\0';
                command = strtok(processed_message, " "); 
                username = strtok(NULL, " "); 
                othername = strtok(NULL, " ");
                other_ip = strtok(NULL, " ");
                printf("other_ip: %s\n", other_ip);
                kill(front_end, SIGKILL);
                printf("Invitation for game received from %s!\n", othername);
                printf("    ...accept? (y/n): ");
                scanf("%s", &accept_call);
                getchar();
                if (accept_call == 'y')
                {
                    memset((void *) server_message, 0, sizeof(server_message));
                    sprintf(server_message, "%s %s %s", ACK_accept, username, othername);
                    server_message[strlen(server_message)] = '\0';
                    write(sender_pipe[1], (void *) server_message, (size_t) sizeof(server_message));
                    own_addr.sin_addr.s_addr = inet_addr(other_ip);
                    sleep(2);
                    if ((player_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1 )
                    {
                        printf("Error: socket not created\n");
                        return -1;
                    }
                    try_c = 9;
                    while (try_c > 0)
                    {
                        if ((connect(player_fd, (struct sockaddr *) &own_addr, sizeof(own_addr))) == -1)
                        {
                            printf("Error: connect failed\n");
                            sleep(2);
                            try_c--;
                        }
                        else
                        {
                            try_c = -1;
                        }
                    }
                    if (try_c == 0)
                    {
                        printf("Error: connect failed\n");
                    }
                    else
                    {
                        printf("Connect success.\n");
                        if (send(player_fd, (const char *) &CONNECT, sizeof(CONNECT), 0) == -1)
                        {
                            printf("Error: send failed!\n");
                            return -1;
                        }
                        printf("    enviou CONNECT %d\n", CONNECT);
                        if (recv(player_fd, &ACK_NACK, sizeof(ACK_NACK), 0) == -1)
                        {
                            printf("Error: recv failed!\n");
                            return -1;
                        }
                        if (ACK_NACK == 1)
                        {
                            printf("    Alcançou o outro jogador!.\n");
                        }
                    }
                    front_end = front_end_process(back_end_pipe[0], front_end_pipe[1]); 
                    if (front_end == 0)
                    {
                        return 0;
                    }
                }
                else
                {
                    write(sender_pipe[1], (void *) NACK_accept, (size_t) sizeof(NACK_accept));
                } 
            }
            else if (!strncmp (server_message, ACK_accept, sizeof(ACK_accept)))
            {
                write(back_end_pipe[1], (void *) ACK_accept, (size_t) sizeof(ACK_accept));
                if ((listen_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1 )
                {
                    printf("Error: socket not created");
                    exit(EXIT_FAILURE);
                }
                if (bind(listen_fd, (struct sockaddr *) &own_addr, sizeof(own_addr)) == -1)
                {
                    printf("Error: listen_fd bind failed");
                    exit(EXIT_FAILURE);
                }
                if ((listen(listen_fd, 10)) == -1)
                {
                    printf("Error: listen failed");
                    exit(EXIT_FAILURE);
                }
                len = sizeof(own_addr);
                if ((player_fd = accept(listen_fd, (struct sockaddr*)&own_addr, (socklen_t *) &len)) == -1)
                {
                    printf("Error: accept failed");
                    exit(EXIT_FAILURE);
                }
                if(recv(player_fd, (void *) &CONNECT, sizeof(CONNECT), 0) == -1) 
                {
                    printf("Error: recv failed");
                    exit(EXIT_FAILURE);
                }
                if (CONNECT)
                {
                    ACK_NACK = 1;
                    if (send(player_fd, (void *) &ACK_NACK, sizeof(ACK_NACK), 0) == -1)
                    {
                        printf("Error: send failed");
                        exit(EXIT_FAILURE);
                    }
                    printf("[TCP client connected]\n");
                }
            }
        /*  Server Down  */
            else if (!strncmp (server_message, Server_down, sizeof(Server_down)))
            {
                printf("\n%s\n", server_message);
                write (back_end_pipe[1], (void *) server_message, (size_t) sizeof(server_message));
                kill (sender, SIGKILL);
                kill (listener, SIGKILL);
                sleep (2);
                kill (front_end, SIGKILL);
                return 0; 
            } 
        /*  Reconnect  procedure*/
            else if (!strncmp(server_message, Reconnect, sizeof(Reconnect)))
            {
                printf("\n%s\n", server_message);
                write (back_end_pipe[1], (void *) server_message, (size_t) sizeof(server_message));
                kill(sender, SIGKILL);
                kill(listener, SIGKILL);
                kill(front_end, SIGKILL);
                close(client_sockfd);

                try_c = 0;
                client_sockfd = -1;
                while (client_sockfd == -1 && try_c < 59)
                {
                    client_sockfd = Connect_Procedure(is_udp, client_sockfd, &serv_addr); 
                    sleep(3);
                    try_c++;
                    printf("...attempt %d/60: ", try_c);
                }
                if(client_sockfd == -1) 
                {
                    printf("\n%s\n", Server_down);
                    exit(EXIT_SUCCESS);
                }
                else 
                {
                    printf("Connection restabilished!\n");
                    /* 
                        Initialize auxiliars processes
                    */
                    listener  = listener_process(client_sockfd, is_udp, &serv_addr, listener_pipe[1]);
                    sender    = sender_process(client_sockfd, is_udp, &serv_addr, sender_pipe[0]);
                    front_end = front_end_process(back_end_pipe[0], front_end_pipe[1]); 
                    if (listener == 0 || sender == 0 || front_end == 0)
                    {
                        return 0;
                    }
                }
                printf("...Back to normal with server!\n");
            }
            else
            {
                write(back_end_pipe[1], (void *) server_message, (size_t) sizeof(server_message));
                usleep(50000);
            }
        }
    }
} 

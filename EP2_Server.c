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

/*
    TCP CLiente:  socket[] -> connect[] -> receive[]
    UDP Cliente:  socket[]              -> sendto[]

    TCP Servidor: socket[] -> bind[] -> listen[] -> accept[]
    UDP Servidor: socket[] -> bind[]             -> recvfrom[]
*/


void master_processor(int player_rd, char * client_message)
{
    char ACK_new_user[21]  = "...new user created!";
    char NACK_new_user[19] = "...new user failed";
    char ACK_in_user[11]   = "...logged!";
    char NACK_in_user[47]  = "...not logged, username or password incorrect ";
    char ACK_out_user[15]  = "...logged out!";
    char NACK_out_user[23] = "...error: still logged";

    unsigned char client_message_copy[64];
    unsigned char * user, * pass, * command, * token;
    char event[64];
    printf("Master receive from player1_wr: %s\n", client_message);
    strncpy(client_message_copy, client_message, strlen(client_message));
    client_message_copy[strlen(client_message)] = '\0';
    command = strtok(client_message_copy, " ");
/*  NEW    _______________________________________________________________________________*/
    if (!strncmp(command, "new", 3)) 
    {
        printf("Master: receive NEW\n");
        user = strtok(NULL, " ");
        pass = strtok(NULL, " ");
        printf("Master:     user %s\n", user);
        printf("Master:     pass %s\n", pass);
        if(insert_user(user, pass)) {
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
/*  PASS    ______________________________________________________________________________*/
    else if (!strncmp(command, "pass", 4)) 
    {
        /* code */
    }
/*  IN    ________________________________________________________________________________*/
    else if (!strncmp(command, "in", 2)) 
    {
        printf("Master: receive IN\n");
        log_event("log requested.");
        user = strtok(NULL, " ");
        pass = strtok(NULL, " ");
        printf("Master:     user %s\n", user);
        printf("Master:     pass %s\n", pass);
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
/*  HALLOFFAME    ________________________________________________________________________*/
    else if (!strncmp(command, "halloffame", 10)) 
    {
        /* code */
    }
/*  l    _________________________________________________________________________________*/
    else if (!strncmp(command, "l", 1)) 
    {
        /* code */
    }
/*  CALL    ______________________________________________________________________________*/
    else if (!strncmp(command, "pass", 4)) 
    {
        /* code */
    }
/*  PLAY    ______________________________________________________________________________*/
    else if (!strncmp(command, "play", 4)) 
    {
        /* code */
    }
/*  DELAY    _____________________________________________________________________________*/
    else if (!strncmp(command, "delay", 5)) 
    {
        /* code */
    }
/*  OVER    _____________________________________________________________________________*/
    else if (!strncmp(command, "over", 4)) 
    {
        /* code */
    }
/*  OUT    _____________________________________________________________________________*/
    else if (!strncmp(command, "out", 3)) 
    {
        user = strtok(NULL, " ");
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
/*  BYE    _____________________________________________________________________________*/
    else if (!strncmp(command, "bye", 3)) 
    {
        /* code */
    }  
}

/*****************************************************************************************************/
/*    MAIN                                                                                           */
/*****************************************************************************************************/
int main(int argc, char ** argv)
{
    uint16_t port = (uint16_t)atoi(argv[1]);
    uint16_t aux_udp_port = (uint16_t) (atoi(argv[1]) + 1)%60535 + 5000;
    printf("Aux port: %d\n", aux_udp_port);

    int player1_wr[2];
    int player1_rd[2];
    int player2_wr[2];
    int player2_rd[2];
    pipe(player1_wr);
    pipe(player1_rd);
    pipe(player2_wr);
    pipe(player2_rd);

    int listen_fd, tcp_fd, udp_fd; 
    int max_clients = 2;
    struct sockaddr_in serv_addr, serv_addr2, clie_addr; 
    int player_id;

    FILE* wr;
    FILE* rd;

    /*Protocolos: */
    unsigned char CONNECT;
    unsigned char ACK_NACK;
    uint16_t CHANGE_PORT;

    CONNECT = 0;

    srand(time(NULL));

    memset((void *)&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);


    if ((listen_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1 )
    {
        printf("Error: socket not created");
        exit(EXIT_FAILURE);
    }
    if ((udp_fd = socket(AF_INET, SOCK_DGRAM, 0)) == -1 )
    {
        printf("Error: socket not created");
        exit(EXIT_FAILURE);
    }
    
    if (bind(listen_fd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) == -1)
    {
        printf("Error: listen_fd bind failed");
        exit(EXIT_FAILURE);
    }
    if (bind(udp_fd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) == -1)
    {
        printf("Error:udp_fd bind failed");
        exit(EXIT_FAILURE);
    }

    if ((listen(listen_fd, 10)) == -1)
    {
        printf("Error: listen failed");
        exit(EXIT_FAILURE);
    }
    
    /* poll():  performs a similar task to select(2): it waits for one of a set
       of file descriptors to become ready to perform I/O.*/
    struct pollfd fd[2];
    int ret;
    memset(fd, 0 , sizeof(fd));

    int n_clients = 0;
    
    struct client_info player1;
    struct client_info player2;

    socklen_t len;
    ssize_t nbytes;
    unsigned char client_message[64];

    pid_t master;
    if ((master = fork()) == -1)
    {
        printf("Error: master fork failed");
        exit(EXIT_FAILURE);
    }
    if (!master) // Master auxiliar
    {
        while(n_clients < 2) 
        {

            fd[0].fd = udp_fd;
            fd[0].events = POLLIN;
    
            fd[1].fd = listen_fd;
            fd[1].events = POLLIN;

            ret = poll(fd, 2, 0);
            if (ret == -1) {
                perror ("poll");
                return 1;
            }
            if ((fd[0].revents == POLLIN) && fd[0].fd == udp_fd) 
            { 
                printf("Server Poll: ...socket UDP chamando\n");
                len = sizeof(serv_addr);
                if(recvfrom(udp_fd, (void *) &CONNECT, sizeof(CONNECT), 0, 
                                    (struct sockaddr *) &serv_addr, (socklen_t *) &len) == -1) 
                {
                    printf("Error: udp_fd pool recvfrom failed");
                    exit(EXIT_FAILURE);
                }
                if (CONNECT)
                {
                    ACK_NACK = 1;
                    if (sendto(udp_fd, (void *) &ACK_NACK, sizeof(ACK_NACK), 0, 
                                    (const struct sockaddr *) &serv_addr, len) == -1)
                    {
                        printf("Error: sendto failed");
                        exit(EXIT_FAILURE);
                    }
                    printf("[UDP client connected]\n");
                }
                else 
                {
                    close(udp_fd);
                    continue;
                }
                if (n_clients) CHANGE_PORT = htons(aux_udp_port + 1);
                else CHANGE_PORT = htons(aux_udp_port);;
                if (sendto(udp_fd, (void *) &CHANGE_PORT, sizeof(CHANGE_PORT), 0, 
                                (const struct sockaddr *) &serv_addr, len) == -1)
                {
                    printf("Error: sendto failed");
                    exit(EXIT_FAILURE);
                }
                if ((player_id = fork()) == -1)
                {
                    printf("Error:udp fork failed");
                    exit(EXIT_FAILURE);
                }
                if (player_id == 0)
                {
                    close(udp_fd);
                    close(listen_fd);
                    
                    if(n_clients) udp_client_handler(player2_rd[0], player2_wr[1], aux_udp_port + 1);
                    else udp_client_handler(player1_rd[0], player1_wr[1], aux_udp_port);
                    return 0;
                }
                n_clients++;
            }
            if ((fd[1].revents == POLLIN) && (fd[1].fd == listen_fd)) 
            {
                printf("Server Poll: ...socket TCP chamando\n");
                len = sizeof(serv_addr);
                if ((tcp_fd = accept(listen_fd, (struct sockaddr*)&serv_addr, (socklen_t *) &len)) == -1)
                {
                    printf("Error: accept failed");
                    exit(EXIT_FAILURE);
                }
                if(recv(tcp_fd, (void *) &CONNECT, sizeof(CONNECT), 0) == -1) 
                {
                    printf("Error: recv failed");
                    exit(EXIT_FAILURE);
                }
                if (CONNECT)
                {
                    ACK_NACK = 1;
                    if (send(tcp_fd, (void *) &ACK_NACK, sizeof(ACK_NACK), 0) == -1)
                    {
                        printf("Error: send failed");
                        exit(EXIT_FAILURE);
                    }
                    printf("[TCP client connected]\n");
                }
                if ((player_id = fork()) == -1)
                {
                    printf("Error: tcp fork failed");
                    exit(EXIT_FAILURE);
                }
                if (player_id == 0)
                {
                    close(listen_fd);
                    close(udp_fd);
                    if(n_clients) tcp_client_handler(player2_rd[0], player2_wr[1], tcp_fd, &serv_addr);
                    else tcp_client_handler(player1_rd[0], player1_wr[1], tcp_fd, &serv_addr);
                    return 0;
                }
                close(tcp_fd);
                n_clients++;    
            }
        }
        printf("Master aux: %d players ...bye!\n", n_clients);
        return 0;
    }

    close(udp_fd);
    close(listen_fd);

    char ACK_new_user[21]  = "...new user created!";
    char NACK_new_user[19] = "...new user failed";
    char ACK_in_user[11]   = "...logged!";
    char NACK_in_user[47]  = "...not logged, username or password incorrect ";
    char ACK_out_user[15]  = "...logged out!";
    char NACK_out_user[23] = "...error: still logged";

    while(1) 
    {
        fd[0].fd = player1_wr[0];
        fd[0].events = POLLIN;

        fd[1].fd = player2_wr[0];
        fd[1].events = POLLIN;

        ret = poll(fd, 2, 0);
        if (ret == -1) {
            perror ("poll");
            return 1;
        }
         
        if ((fd[0].revents == POLLIN) && fd[0].fd == player1_wr[0]) 
        {
            /* Atualiza banco de dados com requisições do 
            processo do servidor referente a player 1*/
            printf("Master poll: pipe do Player1!\n");
            read(player1_wr[0], client_message, sizeof(client_message));
            master_processor(player1_rd[1], client_message);
            memset(client_message, 0, sizeof(client_message));

        }
        else if ((fd[1].revents == POLLIN) && fd[1].fd == player2_wr[0])
        {
            printf("Master poll: pipe do Player2!\n");
            /* Atualiza banco de dados com requisições do 
               processo do servidor referente a player 2*/
            read(player2_wr[0], client_message, sizeof(client_message));
            master_processor(player2_rd[1], client_message);
            memset(client_message, 0, sizeof(client_message));
        }
    
       
    }
}

/*

BIBLIOGRAFIA

    - Cooper, M. (2014). Advanced bash scripting guide: An in-depth exploration 
    of the art of shell scripting (Vol. 1). Domínio público, 10 Mar 2014.
    
    - Kurose, J. and Ross, K., 2016. Computer Networking. Harlow, United 
    Kingdom: Pearson Education Canada.

Sites:

    Fabio Busatto:   https://tldp.org/HOWTO/TCP-Keepalive-HOWTO/programming.html
    Codingunit:      https://www.codingunit.com/c-tutorial-deleting-and-renaming-a-file
    Oasis:           https://docs.oasis-open.org/mqtt/mqtt/v3.1.1/mqtt-v3.1.1.html 
    Wikipedia:       https://en.wikipedia.org 
    Michael Kerrisk: https://man7.org 
    Die(Dice):       https://linux.die.net/ 

*/
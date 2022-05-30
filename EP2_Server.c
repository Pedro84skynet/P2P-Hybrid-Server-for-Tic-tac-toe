/******************************************************************************
 *  Compilation:  (Use make)
 *  Execution:    ./EP2_Server port_number
 *
 *  - port_number: port number used to connect in EP2_Servidor.
 *
 *  DESCRIPTION
 *
 *  A brief description about what is implemented.
 *
 *  PROJECT DECISIONS OR UNFINISHED TASKS (?)
 *
 *  List them bellow
 *
 *  - Implement BYE and return safely
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

static bool DEBUG = false;

/*
    TCP CLiente:  socket[] -> connect[] -> receive[]
    UDP Cliente:  socket[]              -> sendto[]

    TCP Servidor: socket[] -> bind[] -> listen[] -> accept[]
    UDP Servidor: socket[] -> bind[]             -> recvfrom[]
*/

/*****************************************************************************************************/
/*    MAIN                                                                                           */
/*****************************************************************************************************/
int main(int argc, char ** argv)
{
    uint16_t port = (uint16_t)atoi(argv[1]);
    uint16_t aux_udp_port = (uint16_t) (atoi(argv[1]) + 1)%60535 + 5000;

    for (int i = 0; i < argc; i++)
    {
        if (!strncmp(argv[i], "-d", 2) || !strncmp(argv[i], "-D", 2))
        {
            DEBUG = true;
        }
    }
    
    if(DEBUG) printf("[Main Process] Aux port: %d\n", aux_udp_port);

    int player1_wr[2];
    int player1_rd[2];
    int player2_wr[2];
    int player2_rd[2];
    pipe(player1_wr);
    pipe(player1_rd);
    pipe(player2_wr);
    pipe(player2_rd);

    int listen_fd, tcp_fd, udp_fd; 
    int mh_return;
    int max_clients = 2;
    struct sockaddr_in serv_addr, serv_addr2, clie_addr; 
    int player_id;

    FILE* wr;
    FILE* rd;

    /*Protocolos: */
    unsigned char CONNECT;
    unsigned char ACK_NACK;
    uint16_t CHANGE_PORT;

    int ip_len;
    char player_ip[16];
    char event[64];

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

            ret = poll(fd, 2, -1);
            if (ret == -1) {
                perror ("poll"); 
                return 1;
            }
            if ((fd[0].revents == POLLIN) && fd[0].fd == udp_fd) 
            { 
                if(DEBUG)printf("[Main Aux] Server Poll: ...socket UDP chamando\n");
                len = sizeof(serv_addr);
                if(recvfrom(udp_fd, (void *) &CONNECT, sizeof(CONNECT), 0, 
                                    (struct sockaddr *) &serv_addr, (socklen_t *) &len) == -1) 
                {
                    printf("Error: udp_fd pool recvfrom failed");
                    exit(EXIT_FAILURE);
                }
                ip_len = strlen(inet_ntoa(serv_addr.sin_addr));
                memset((void *) player_ip, 0, sizeof(player_ip));
                strncpy(player_ip, inet_ntoa(serv_addr.sin_addr), ip_len + 1);
                player_ip[ip_len + 1] = '\0';
                if(DEBUG) printf("[Main Aux]    IP address is: %s\n", player_ip);
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
                    
                    if(n_clients)client_handler(player_ip, true, player2_rd[0], player2_wr[1], aux_udp_port + 1, port, DEBUG);
                    else client_handler(player_ip, true, player1_rd[0], player1_wr[1], aux_udp_port, port, DEBUG);

                    return 0;
                }
                n_clients++;
            }
            if ((fd[1].revents == POLLIN) && (fd[1].fd == listen_fd)) 
            {
                if(DEBUG)printf("[Main Aux] Server Poll: ...socket TCP chamando\n");
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
                ip_len = strlen(inet_ntoa(serv_addr.sin_addr));
                memset((void *) player_ip, 0, sizeof(player_ip));
                strncpy(player_ip, inet_ntoa(serv_addr.sin_addr), ip_len + 1);
                player_ip[ip_len + 1] = '\0';
                if(DEBUG)printf("[Main Aux]    IP address is: %s\n", player_ip);
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
                    if(n_clients) client_handler(player_ip, false, player2_rd[0], player2_wr[1], 0, tcp_fd, DEBUG);
                    else client_handler(player_ip, false, player1_rd[0], player1_wr[1], 0, tcp_fd, DEBUG);

                    return 0;
                }
                close(tcp_fd);
                n_clients++;    
            }
        }
        if(DEBUG)printf("[Main Aux] %d players ...bye!\n", n_clients);
        return 0;
    }

    close(udp_fd);
    close(listen_fd);

    while(1) 
    {
        fd[0].fd = player1_wr[0];
        fd[0].events = POLLIN;

        fd[1].fd = player2_wr[0];
        fd[1].events = POLLIN;

        ret = poll(fd, 2, -1);
        if (ret == -1) {
            printf("Error: poll failed!\n");
            return 1;
        }
         
        if ((fd[0].revents == POLLIN) && fd[0].fd == player1_wr[0]) 
        {
            /* Atualiza banco de dados com requisições do 
            processo do servidor referente a player 1*/
            if(DEBUG) printf("[Main Process] poll: pipe do Player1!\n");
            read(player1_wr[0], client_message, sizeof(client_message));
            if(DEBUG) printf("[Main Process] read from player1_wr[0]: %s\n", client_message);
            mh_return = master_handler(player1_rd[1], client_message, DEBUG);
        /*  CALL    __________________________________________________________________*/
            if (mh_return == 6) // call to other player. 
            {
                if(DEBUG) printf("[Main Process] mh_return == 6\n");
                write(player2_rd[1], client_message, sizeof(client_message));
            }
            if (mh_return == 8) // call to other player.
            {
                if(DEBUG) printf("[Main Process] mh_return == 8\n");
                write(player2_rd[1], ACK_accept, sizeof(ACK_accept));
            }
            if (mh_return == 12) // call to other player.
            {
                if(DEBUG) printf("[Main Process] mh_return == 12\n");
                write(player2_rd[1], You_lose, sizeof(You_lose));
            }
            if (mh_return == 13) // call to other player.
            {
                if(DEBUG) printf("[Main Process] mh_return == 12\n");
                write(player2_rd[1], Draw, sizeof(Draw));
            }
            memset(client_message, 0, (size_t) sizeof(client_message));
        }
        else if ((fd[1].revents == POLLIN) && fd[1].fd == player2_wr[0])
        {
            if(DEBUG) printf("[Main Process] poll: pipe do Player2!\n");
            /* Atualiza banco de dados com requisições do 
               processo do servidor referente a player 2*/
            read(player2_wr[0], client_message, sizeof(client_message));
            if(DEBUG) printf("[Main Process] read from player2_wr[0]: %s\n", client_message);
            mh_return = master_handler(player2_rd[1], client_message, DEBUG);
        /*  CALL    __________________________________________________________________*/
            if (mh_return == 6) // call to other player.
            {
                if(DEBUG) printf("[Main Process] mh_return == 6\n");
                write(player1_rd[1], client_message, sizeof(client_message));
            }
            if (mh_return == 8) // call to other player.
            {
                if(DEBUG) printf("[Main Process] mh_return == 8\n");
                write(player1_rd[1], ACK_accept, sizeof(ACK_accept));
            }
            if (mh_return == 12) // call to other player.
            {
                if(DEBUG) printf("[Main Process] mh_return == 12\n");
                write(player1_rd[1], You_lose, sizeof(You_lose));
            }
            if (mh_return == 13) // call to other player.
            {
                if(DEBUG) printf("[Main Process] mh_return == 12\n");
                write(player1_rd[1], Draw, sizeof(Draw));
            }
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



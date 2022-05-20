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

/*
    TCP CLiente:  socket[] -> connect[] -> receive[]
    UDP Cliente:  socket[]              -> sendto[]

    TCP Servidor: socket[] -> bind[] -> listen[] -> accept[]
    UDP Servidor: socket[] -> bind[]             -> recvfrom[]
*/

struct client_info {
    char ip[16];
    uint16_t port;
    uint16_t P2P_port;
    bool main;
};

void log_event(char *event) 
{
    FILE* log;
    time_t s = time(NULL);
    struct tm t = *localtime(&s);

    log = fopen("log.txt", "a");
    fprintf(log, "[%d//%02d//%02d - %02d:%02d:%02d]: %s\n",
                t.tm_year + 1900, t.tm_mon + 1, t.tm_mday, 
                t.tm_hour, t.tm_min, t.tm_sec, event);
    fclose(log);
}
int change_data(char *username, int cod) 
{
    char *token, *user, *password; 
    int n_vic, is_on, in_game;
    in_game = 2;
    char line[64], line_tokenized[64];
    int score;
    FILE* fp_db;
    FILE* fp_new_db;
    fp_db = fopen("database.txt", "r");
    fp_new_db = fopen("prov_database.txt", "a");
    while (fgets(line, 64, fp_db))
    {
        line[strlen(line) - 1] = '\0'; 
        printf("Reading line: %s\n", line);
        strncpy(line_tokenized, line, strlen(line));
        printf("line_tokenized line: %s\n", line_tokenized);
        token = strtok(line_tokenized, " ");
        if (!strncmp(token, username, strlen(username) - 1))
        {
            password = strtok(NULL, " ");
            score = atoi(strtok(NULL, " "));
            if (cod == 1)
            {
                score++;
            }
            is_on = atoi(strtok(NULL, " "));
            if (cod == 2) 
            {
                is_on = (is_on == 0) ?  1 :  0;

            }
            in_game = atoi(strtok(NULL, "\n"));
            if (cod == 3) 
            {
                in_game = (in_game == 0) ?  1 :  0;
            }
            printf("    score: %d, is_on: %d, in_game: %d\n",score, is_on, in_game);
            bzero((void*) line, 64);
            memset((void*) line, 0, 64);
            sprintf(line, "%s %s %d %d %d", username, password, score, is_on, in_game );
            printf("    new line in database: %s\n", line);
            fprintf(fp_new_db, "%s\n", line);
        } 
        else
        {
            fprintf(fp_new_db, "%s\n", line);
        }
        memset((void*) line, 0, 64);
        memset((void*) line_tokenized, 0, 64);
    }
    fclose(fp_db);
    fclose(fp_new_db);
    while (remove("database.txt") != 0)
    {
        printf("Erro: arquivo %s não deletado.\n", "database.txt");
        sleep(1);
    }
    printf("Arquivo %s  deletado.\n", "database.txt");
    while (rename("prov_database.txt", "database.txt") != 0)
    {
        printf("Erro: arquivo %s não atualizado.\n", "database.txt");
        sleep(1);
    }
    printf("Arquivo %s  atualizado.\n", "database.txt");
    return 0;  
}
int change_pass(char *username, char *old_pass, char *new_pass) 
{
    char *token, *user, *password, *n_vic, *is_on, *in_game;
    char line[64], line_tokenized[64];
    FILE* fp_db;
    FILE* fp_new_db;
    fp_db = fopen("database.txt", "r");
    fp_new_db = fopen("prov_database.txt", "a");
    while (fgets(line, 64, fp_db))
    {
        line[strlen(line) - 1] = '\0'; 
        printf("Reading line: %s\n", line);
        strncpy(line_tokenized, line, strlen(line));
        printf("line_tokenized line: %s\n", line_tokenized);
        token = strtok(line_tokenized, " ");
        if (!strncmp(token, username, strlen(username) - 1))
        {
            token = strtok(NULL, " ");
            if (strncmp(token, old_pass, strlen(old_pass) - 1))
            {
                printf("Password antigo incorreto!\n");
                fclose(fp_db);
                fclose(fp_new_db);
                return 3;
            }
            else
            {
                printf("Password antigo correto!\n");
                n_vic = strtok(NULL, " ");
                is_on = strtok(NULL, " ");
                in_game = strtok(NULL, " ");
                sprintf(line, "%s %s %s %s %s", username, new_pass, n_vic, is_on, in_game);
                line[strlen(line) - 1] = '\0'; 
                printf("    new line in database: %s\n", line);
                fprintf(fp_new_db, "%s\n", line);
            }  
        } 
        else
        {
            fprintf(fp_new_db, "%s\n", line);
        }
    } 
    if(remove("database.txt") == 0)
    {
        printf("Arquivo %s  deletado.\n", "database.txt");
    } 
    else
    {
        printf("Erro: arquivo %s não deletado.\n", "database.txt");
        fclose(fp_db);
        fclose(fp_new_db);
        return 1;
    }
    if(rename("prov_database.txt", "database.txt") == 0)
    {
        printf("Arquivo %s  atualizado.\n", "database.txt");
        fclose(fp_db);
        fclose(fp_new_db);
        return 0;
    } 
    else
    {
        printf("Erro: arquivo %s não atualizado.\n", "database.txt");
        fclose(fp_db);
        fclose(fp_new_db);
        return 2;
    }  
}
bool is_online(char *username) 
{
    FILE *fp;
    char line[64];
    char *token, *n_vic, *password, *is_on;
    fp = fopen("database.txt", "r");
    while (fgets(line, 64, fp))
    {
        token = strtok(line, " ");
        if (!strncmp(username, token, sizeof(username)))
        {
            password = strtok(NULL, " ");
            n_vic = strtok(NULL, " ");
            is_on = strtok(NULL, " ");
            if (!strncmp(is_on, "1", 1))
            {
                return true;
            } 
            else
            {
                return false;
            }
        }
    }
    return false;
}


/*****************************************************************************************************/
/*    MAIN                                                                                           */
/*****************************************************************************************************/
int main(int argc, char ** argv)
{
    uint16_t port = (uint16_t)atoi(argv[1]);
    uint16_t aux_udp_port = (uint16_t) (atoi(argv[1]) + 1)%60536 + 5000;
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
                if(recvfrom(fd[0].fd, (void *) &CONNECT, sizeof(CONNECT), 0, 
                                    (struct sockaddr *) &serv_addr, (socklen_t *) &len) == -1) 
                {
                    printf("Error: udp_fd pool recvfrom failed");
                    exit(EXIT_FAILURE);
                }
                if (CONNECT)
                {
                    ACK_NACK = 1;
                    if (sendto(fd[0].fd, (void *) &ACK_NACK, sizeof(ACK_NACK), 0, 
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
                printf("    First UDP client ...sending new port\n");
                if (n_clients) CHANGE_PORT = htons(aux_udp_port);
                else CHANGE_PORT = 0;
                if (sendto(fd[0].fd, (void *) &CHANGE_PORT, sizeof(CHANGE_PORT), 0, 
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
                    n_clients = 2;
                    
                    if(n_clients) udp_client_handler(player1_rd[0], player1_wr[1], aux_udp_port);
                    else udp_client_handler(player2_rd[0], player2_wr[1], port);
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
                    n_clients = 2;
                    if(!n_clients) tcp_client_handler(player1_rd[0], player1_wr[1], tcp_fd, &serv_addr);
                    else tcp_client_handler(player2_rd[0], player2_wr[1], tcp_fd, &serv_addr);
                    return 0;
                }
                close(tcp_fd);
                n_clients++;    
            }
        }

    }
    
    while(1) 
    {
        fd[0].fd = player1_rd[0];
        fd[0].events = POLLIN;

        fd[1].fd = player2_rd[0];
        fd[1].events = POLLIN;

        ret = poll(fd, 2, 0);
        if (ret == -1) {
            perror ("poll");
            return 1;
        }

        if ((fd[0].revents & POLLIN) && fd[0].fd == player1_rd[0]) 
        {
            printf("Master poll: pipe do Player1!\n");
            /* Atualiza banco de dados com requisições do 
               processo do servidor referente a player 1*/
        }
        if ((fd[1].revents & POLLIN) && fd[1].fd == player2_rd[0])
        {
            printf("Master poll: pipe do Player1!\n");
            /* Atualiza banco de dados com requisições do 
               processo do servidor referente a player 2*/
        }
    }
}

/*

BIBLIOGRAFIA

https://tldp.org/HOWTO/TCP-Keepalive-HOWTO/programming.html
https://www.codingunit.com/c-tutorial-deleting-and-renaming-a-file

*/
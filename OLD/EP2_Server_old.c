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
/*    udp_client_handler                                                                             */
/*    Recebe commandos de um cliente UDP                                                             */
/*****************************************************************************************************/
int udp_client_handler(int fd, struct sockaddr_in * addr, 
                        int pipe_fd[2], struct client_info *player) 
{
    char user_input[64], line[64], event[64];
    char *command, *username, *password;
    char *old_pass, *new_pass;
    char *user, *pass, *token1, *token2, *token3, *result;
    char c;
    int n_bytes;
    bool access_denied = true;
    bool calling_self = false;

    unsigned char ACK_NACK;

    socklen_t len = sizeof(addr);;
    FILE* fpointer;

    struct pollfd poll_fd[2];
    int ret;
    
    struct client_info * player2 = malloc(sizeof(struct client_info));

    log_event("UDP client connected");
    while (1)
    {
        poll_fd[0].fd = fd;
        poll_fd[0].events = POLLIN;
        poll_fd[1].fd = pipe_fd[0];
        poll_fd[1].events = POLLIN;

        memset((void *)user_input, 0, 64);
        ret = poll(poll_fd, 2,  0);
        if (ret == -1)
        {
            printf("Error: poll from handler failed");
            exit(EXIT_FAILURE);
        }
        if ((poll_fd[1].revents != 0) && (poll_fd[1].fd == pipe_fd[0]) 
                                      && !calling_self)
        {
            read(pipe_fd[0], (void *) player2, sizeof(struct client_info));
            printf("poll: Recebeu mensagem via pipe_fd\n");
            char * inv_game = "game";
            if(sendto(fd, (void *) inv_game, sizeof(inv_game), 0, 
                                        (struct sockaddr *) addr, len) == -1) 
            {
                printf("Error: recvfrom failed");
                exit(EXIT_FAILURE);
            }
            if(sendto(fd, (void *) player2, sizeof(struct client_info), 0, 
                                        (struct sockaddr *) addr, len) == -1) 
            {
                printf("Error: recvfrom failed");
                exit(EXIT_FAILURE);
            }
        }
        // else if(ret == 0)
        // {

        // }
        else if ((poll_fd[0].revents == POLLIN) && (poll_fd[0].fd == fd))
        {
            len = sizeof(addr);
            printf("poll: Recebeu mensagem via socket\n");
            if(recvfrom(fd, (void *) user_input, sizeof(user_input), 0, 
                                    (struct sockaddr *) addr, (socklen_t *) &len) == -1) 
            {
                printf("Error: handler recvfrom 1 failed");
                exit(EXIT_FAILURE);
            } 
            printf("    user input: %s\n", user_input);
            command = strtok(user_input, " ");
            /* NEW *---------------------------------------------------------------------*/
            if (!strncmp(command, "new", 3))
            {
                log_event("new user creation required.");
                username = strtok(NULL, " ");
                password = strtok(NULL, " ");
                fpointer = fopen("database.txt", "a");
                fprintf(fpointer,"%s %s %d %d %d\n", username, password, 0, 0, 0);
                sprintf(event,"new user %s created.", username);
                log_event(event);
                fclose(fpointer);
                printf("sending to port: %d\n", ntohs((*addr).sin_port));
                ACK_NACK = 1;
                if((n_bytes = sendto(fd, (void *) &ACK_NACK, sizeof(ACK_NACK), 0, 
                                        (struct sockaddr *) addr, len)) == -1) 
                {
                    printf("Error: sendto failed");
                    exit(EXIT_FAILURE);
                }
                printf("ACK_NACK sended %d\n", n_bytes);
            }
            /* PASS *--------------------------------------------------------------------*/
            else if (!strncmp(command, "pass", 4))
            {
                if (access_denied)
                {
                    printf("    Não está logado.");
                    continue;
                }
                char * old_pass = strtok(NULL, " ");
                char * new_pass = strtok(NULL, " ");
                if (!change_pass(username, old_pass, new_pass))
                {
                    ACK_NACK = 1;
                }
                else
                {
                    ACK_NACK = 0;
                }
                if((n_bytes = sendto(fd, (void *) &ACK_NACK, sizeof(ACK_NACK), 0, 
                                        (struct sockaddr *) addr, len)) == -1) 
                {
                    printf("Error: sendto failed");
                    exit(EXIT_FAILURE);
                }
            }
            /* IN *----------------------------------------------------------------------*/
            else if (!strncmp(command, "in", 2))
            {
                if (!access_denied)
                {
                    printf("    Já está logado.");
                    continue;
                }
                log_event("login required.");
                command = strtok(NULL, " ");
                username = malloc(sizeof(strlen(command)));
                strncpy(username, command, strlen(command));
                username[strlen(command)] = '\0';
                command = strtok(NULL, " ");
                password = malloc(sizeof(strlen(command)));
                strncpy(password, command, strlen(command));
                password[strlen(command) - 1] = '\0';
                if ((fpointer = fopen("database.txt", "r")) == 0)
                {
                    printf("    Banco de dados inexistente.");
                    continue;
                }
                while (fgets(line, 64, fpointer) != NULL && access_denied)
                {
                    line[strlen(line) - 1] = '\0'; 
                    c = 1;
                    command = strtok(line, " ");
                    if (strncmp(command, username, strlen(username) + 1))
                    {
                        c = 0;         
                    }
                    command = strtok(NULL, " ");
                    if (strncmp(command, password, strlen(password)))
                    {
                        c = 0;               
                    }
                    if(c == 1) access_denied = false;
                }
                if (access_denied) 
                {
                    printf("\nAccess Denied!\n\n");
                    log_event("Access denied username or password incorrect.");
                    ACK_NACK = 0;
                    if((n_bytes =sendto(fd, (void *) &ACK_NACK, sizeof(ACK_NACK), 0, 
                                        (struct sockaddr *) addr, len)) == -1) 
                    {
                        printf("Error: recvfrom failed");
                        exit(EXIT_FAILURE);
                    }
                }
                else
                {
                    sprintf(event,"access granted to %s.", username);
                    log_event(event);
                    change_data(username, 2);
                    ACK_NACK = 1;
                    if((n_bytes = sendto(fd, (void *) &ACK_NACK, sizeof(ACK_NACK), 0, 
                                        (struct sockaddr *) addr, len)) == -1) 
                    {
                        printf("Error: recvfrom failed");
                        exit(EXIT_FAILURE);
                    }
                    printf("ACK_NACK sended %d\n", n_bytes);
                }
                fclose(fpointer);
            }
            /* HALLOFFAME *--------------------------------------------------------------*/
            else if (!strncmp(command, "halloffame", 10))
            {
                if (access_denied)
                {
                    printf("    Não está logado.");
                    continue;
                }
                if ((fpointer = fopen("database.txt", "r")) == 0)
                {
                    printf("    Banco de dados inexistente.");
                    continue;
                }
                while (fgets(line, 64, fpointer))
                {
                    user = strtok(line, " ");
                    pass = strtok(NULL, " ");
                    token1 = strtok(NULL, " ");
                    token2 = strtok(NULL, " ");
                    token3 = strtok(NULL, " ");
                    sprintf(line, "%s %s", user, token1);
                    //line[strlen(line)] = '\0';
                    printf("line sended: %s strlen: %ld\n",line, strlen(line));
                    if((n_bytes = sendto(fd, (void *) line, strlen(line), 0, 
                                        (struct sockaddr *) addr, len)) == -1) 
                    {
                        printf("Error: recvfrom failed\n");
                        exit(EXIT_FAILURE);
                    }
                    memset((void *)line, 0, 64);
                }
                line[0] = 0;
                if((n_bytes = sendto(fd, (void *) line, 1, 0, 
                                        (struct sockaddr *) addr, len)) == -1) 
                {
                    printf("Error: recvfrom failed\n");
                    exit(EXIT_FAILURE);
                }
                printf("    line[0] sended!\n");
                fclose(fpointer);
            }
            /* l *-----------------------------------------------------------------------*/
            else if (!strncmp(command, "l", 1))
            {
                if (access_denied)
                {
                    printf("    Não está logado.");
                    continue;
                }
                if ((fpointer = fopen("database.txt", "r")) == 0)
                {
                    printf("    Banco de dados inexistente.");
                    continue;
                }
                memset((void *)line, 0, 64);
                while (fgets(line, 64, fpointer))
                {
                    user = strtok(line, " ");
                    pass = strtok(NULL, " ");
                    token1 = strtok(NULL, " ");
                    token2 = strtok(NULL, " ");
                    token3 = strtok(NULL, " ");
                    if (!strncmp(token2, "1", 1))
                    {
                        if (!strncmp(token3, "1", 1)) sprintf(line, "%s %s", user, "sim");
                        else sprintf(line, "%s | %s", user, "não");
                        line[strlen(line)] = '\0';
                        if((n_bytes = sendto(fd, (void *) line, strlen(line), 0, 
                                            (struct sockaddr *) addr, len)) == -1) 
                        {
                            printf("Error: sendto failed");
                            exit(EXIT_FAILURE);
                        }
                        printf("line sended: %s strlen: %ld\n",line, strlen(line));
                    }
                    memset((void *)line, 0, 64);
                }
                line[0] = 0;
                if((n_bytes = sendto(fd, (void *) line, 1, 0, 
                                        (struct sockaddr *) addr, len)) == -1) 
                {
                    printf("Error: sendto line[0] failed");
                    exit(EXIT_FAILURE);
                }
                printf("    line[0] sended!\n");
                fclose(fpointer);
            }
            /* CALL *--------------------------------------------------------------------*/
            else if (!strncmp(command, "call", 4))
            {
                if (access_denied)
                {
                    printf("    Não está logado.");
                    continue;
                }
                command = strtok(NULL, " ");
                printf("    command: %s\n", command);
                if (!strncmp(command, "accepted", 8))
                {
                    printf("    Partida registrada.\n");
                    change_data(username, 3);
                    continue;
                }
                
                if (is_online(command))
                {
                    ACK_NACK = 1;
                    if(sendto(fd, (void *) &ACK_NACK, sizeof(unsigned char), 0, 
                                            (struct sockaddr *) addr, len) == -1) 
                    {
                        printf("Error: sendto failed\n");
                        exit(EXIT_FAILURE);
                    }
                    write(pipe_fd[1], (void *) player, sizeof(struct client_info));
                    if(sendto(fd, (void *) player, sizeof(struct client_info), 0, 
                                            (struct sockaddr *) addr, len) == -1) 
                    {
                        printf("Error: sendto failed\n");
                        exit(EXIT_FAILURE);
                    }
                    calling_self = true;
                    sleep(1);
                }
                else
                {
                    ACK_NACK = 0;
                    if(sendto(fd, (void *) &ACK_NACK, sizeof(unsigned char), 0, 
                                            (struct sockaddr *) addr, len) == -1) 
                    {
                        printf("Error: sendto failed\n");
                        exit(EXIT_FAILURE);
                    }
                }
            }
            /* DELAY *-------------------------------------------------------------------*/
            else if (!strncmp(command, "delay", 5))
            {
                if (access_denied)
                {
                    printf("    Não está logado.\n");
                    continue;
                }
            }
            /* OVER *--------------------------------------------------------------------*/
            else if (!strncmp(command, "over", 4))
            {
                if (access_denied)
                {
                    printf("    Não está logado.\n");
                    continue;
                }
                change_data(username, 3);
            }
            /* OUT *---------------------------------------------------------------------*/
            else if (!strncmp(command, "out", 3))
            {
                if (access_denied)
                {
                    printf("    Não está logado.\n");
                    continue;
                }
                change_data(username, 2);
                free(username);
                free(password);
                ACK_NACK = 1;
                if(sendto(fd, (void *) &ACK_NACK, sizeof(ACK_NACK), 0, 
                                    (struct sockaddr *) addr, len) == -1) 
                {
                    printf("Error: recvfrom failed.\n");
                    exit(EXIT_FAILURE);
                }
                access_denied = true;
            }
            /* BYE *---------------------------------------------------------------------*/
            else if (!strncmp(command, "bye", 3))
            {
                access_denied = true;
                return 0;
            }
            /* Especiais */
            else if (!strncmp(command, "game_end", 8))
            {
                change_data(username, 1);
                change_data(username, 3);
            }
            else if (!strncmp(command, "playing", 7))
            {
                sleep(1);
                change_data(username, 3);
            }
            else
            {
                printf("commando desconhecido!\n");
            }  
        }
    }
}

/*****************************************************************************************************/
/*    MAIN                                                                                           */
/*****************************************************************************************************/
int main(int argc, char ** argv)
{
    int aux_udp_port = (atoi(argv[1]) + 1)%60536 + 5000;
    printf("Aux port: %d\n", aux_udp_port);
    int pipe_fd_p1[2];
    int pipe_fd_p2[2];
    pipe(pipe_fd_p1);
    pipe(pipe_fd_p2);
    int listen_fd, tcp_fd, udp_fd, udp_fd2; 
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
    serv_addr.sin_port = htons(atoi(argv[1]));
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    memset((void *)&serv_addr2, 0, sizeof(serv_addr2));
    serv_addr2.sin_family = AF_INET;
    serv_addr2.sin_port = htons(aux_udp_port);
    serv_addr2.sin_addr.s_addr = htonl(INADDR_ANY);

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
    struct pollfd fd[4];
    int ret;
    memset(fd, 0 , sizeof(fd));

    int n_clients = 0;
    
    struct client_info player1;
    struct client_info player2;

    socklen_t len;
    ssize_t nbytes;
    unsigned char client_message[64];

    while(n_clients < 2) {

        fd[0].fd = udp_fd;
        fd[0].events = POLLIN;
 
        fd[2].fd = listen_fd;
        fd[2].events = POLLIN;

	    ret = poll(fd, 3, 0);
        if (ret == -1) {
            perror ("poll");
            return 1;
        }
   
        if ((fd[0].revents & POLLIN) && fd[0].fd == udp_fd) { 
            
            len = sizeof(serv_addr);
            if(recvfrom(fd[0].fd, (void *) &CONNECT, sizeof(CONNECT), 0, 
                                (struct sockaddr *) &serv_addr, (socklen_t *) &len) == -1) 
            {
                printf("Error: udp_fd pool recvfrom failed");
                exit(EXIT_FAILURE);
            }
            if (CONNECT == 0)
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
                close(fd[0].fd);
                continue;
            }
            
            int ip_len = strlen(inet_ntoa(serv_addr.sin_addr));
            if (!n_clients)
            {
                printf("First UDP client sending new port\n");
                CHANGE_PORT = htons(aux_udp_port);
                if (sendto(fd[0].fd, (void *) &CHANGE_PORT, sizeof(CHANGE_PORT), 0, 
                                (const struct sockaddr *) &serv_addr, len) == -1)
                {
                    printf("Error: sendto failed");
                    exit(EXIT_FAILURE);
                }
                strncpy(player1.ip, inet_ntoa(serv_addr.sin_addr), ip_len + 1);
                player1.ip[ip_len + 1] = '\0';
                printf("    IP address is: %s\n", player1.ip);
                player1.port = ntohs(serv_addr.sin_port);
                player1.P2P_port = (uint16_t) (rand()%60536 + 5000);
                player1.main = true;
                printf("    port is: %d\n", (int) player1.port);
                if ((player_id = fork()) == -1)
                {
                    printf("Error:udp fork failed");
                    exit(EXIT_FAILURE);
                }
                if (player_id == 0)
                {
                    close(fd[0].fd);
                    close(listen_fd);

                    if ((udp_fd2 = socket(AF_INET, SOCK_DGRAM, 0)) == -1 )
                    {
                        printf("Error: socket 2 not created");
                        exit(EXIT_FAILURE);
                    }
                    if (bind(udp_fd2, (struct sockaddr *) &serv_addr2, sizeof(serv_addr2)) == -1)
                    {
                        printf("Error:udp_fd2 bind failed");
                        exit(EXIT_FAILURE);
                    } 
                    n_clients = 2;
                   
                    udp_client_handler(udp_fd2,  &serv_addr2, pipe_fd_p1, &player1);
                    return 0;
                }
            } 
            else 
            {
                CHANGE_PORT = 0;
                if (sendto(fd[0].fd, (void *) &CHANGE_PORT, sizeof(CHANGE_PORT), 0, 
                                (const struct sockaddr *) &serv_addr, len) == -1)
                {
                    printf("Error: sendto failed");
                    exit(EXIT_FAILURE);
                }
                strncpy(player2.ip, inet_ntoa(serv_addr.sin_addr), ip_len);
                player2.ip[ip_len + 1] = '\0';
                printf("    IP address is: %s\n", player2.ip);
                player2.port = ntohs(serv_addr.sin_port);
                player2.P2P_port = player1.P2P_port;
                player2.main = false;
                printf("    port is: %d\n", (int) player2.port);

                close(listen_fd);
                udp_client_handler(udp_fd, &serv_addr, pipe_fd_p1, &player2);
            }
            n_clients++;
        }

        if ((fd[2].revents & POLLIN) && fd[2].fd == listen_fd) {
            // printf("[New TCP connection]\n");
            // len = sizeof(serv_addr);
            // if ((tcp_fd = accept(fd[2].fd, (struct sockaddr*)&serv_addr, &len)) == -1)
            // {
            //     printf("Error: accept failed");
            //     exit(EXIT_FAILURE);
            // }
            // if(recv(tcp_fd, (void *) &CONNECT, sizeof(CONNECT), 0) == -1)
            // {
            //     printf("Error: recv failed");
            //     exit(EXIT_FAILURE);
            // }
            // CONNECT = client_message[0];
            // if (CONNECT == 0)
            // {
            //     ACK_NACK = 1;
            //     if (send(fd[2].fd, (void *) &ACK_NACK, sizeof(ACK_NACK), 0) == -1)
            //     {
            //         printf("Error: sendto failed");
            //         exit(EXIT_FAILURE);
            //     }
            //     printf("[New TCP connection]\n");
            // }
            // else 
            // {
            //     close(fd[2].fd);
            //     continue;
            // }
            // int ip_len = strlen(inet_ntoa(serv_addr.sin_addr));
            // if (!n_clients)
            // {
            //     strncpy(player1.ip, inet_ntoa(serv_addr.sin_addr), ip_len + 1);
            //     player1.ip[ip_len + 1] = '\0';
            //     printf("    IP address is: %s\n", player1.ip);
            //     player1.port = ntohs(serv_addr.sin_port);
            //     player1.P2P_port = (uint16_t) (rand()%60536 + 5000);
            //     player1.main = true;
            //     printf("    port is: %d\n", (int) player1.port);
            //     if ((player_id = fork()) == -1)
            //     {
            //         printf("Error: tcp fork failed");
            //         exit(EXIT_FAILURE);
            //     }
            //     if (player_id == 0)
            //     {
            //         close(fd[2].fd);

            //         read(pipe_fd1[0], &player2, sizeof(player2));
            //         nbytes = send(tcp_fd, (void *) &player2, sizeof(player2), 0);
            //         n_clients = 2;
            //         while (1)
            //         {
            //             /* code */
            //         }
            //     }    
            // } 
            // else 
            // {
            //     strncpy(player2.ip, inet_ntoa(serv_addr.sin_addr), ip_len);
            //     player2.ip[ip_len + 1] = '\0';
            //     printf("    IP address is: %s\n", player2.ip);
            //     player2.port = ntohs(serv_addr.sin_port);
            //     player2.P2P_port = player1.P2P_port;
            //     player2.main = false;
            //     printf("    port is: %d\n", (int) player2.port);

            //     close(listen_fd);
            //     write(pipe_fd1[1], &player2, sizeof(player2));
            //     nbytes = send(tcp_fd, (void *) &player1, sizeof(player1), 0);
            // }
            // n_clients++;
        }
    }

    while(1) {
        
    }
}

/*

BIBLIOGRAFIA

https://tldp.org/HOWTO/TCP-Keepalive-HOWTO/programming.html
https://www.codingunit.com/c-tutorial-deleting-and-renaming-a-file

*/
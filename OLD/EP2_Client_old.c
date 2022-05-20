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

struct client_info {
    char ip[16];
    uint16_t port;
    uint16_t P2P_port;
    bool main;
};

unsigned char * hash_game(unsigned char * hashtable, unsigned char symbol, 
                            int line, int column) 
{
    int c = 0;
    while(!c) {
        if (hashtable[line*3 + column] == 32)
        {
            hashtable[line*3 + column] = symbol;
            c = 1;
        } 
        else
        {
            printf("Erro: casa já preenchida!\n");
            printf("Jogue novamente: \n");
            printf("linha: ");
            scanf("%d", &line);
            printf("coluna: ");
            scanf("%d", &column);
        }
    }
    return hashtable;
}

void print_hash_table(unsigned char * hashtable) {
    printf("\n");
    printf("   0   1   2\n");
    printf("  ___________\n"); 
    for (int i = 0; i < 3; i++)
    {
        printf("%d|", i);
        for (int j = 0; j < 3; j++)
        {
            printf(" %c |", hashtable[i*3 + j]);
        }
        printf("\n  ___________\n");      
    }
    printf("\n");
}

unsigned char hash_winner(unsigned char * hashtable) {
    for (int i = 0; i < 3; i++)
    {
        if(hashtable[i*3] != 32 && 
            (hashtable[i*3] == hashtable[i*3 + 1]) &&
            (hashtable[i*3] == hashtable[i*3 + 2])) 
        {
            return hashtable[i*3]; 
        }
        if(hashtable[i] != 32 && 
            (hashtable[i] == hashtable[i + 3]) && 
            (hashtable[i] == hashtable[i + 6])) 
        {
            return hashtable[i]; 
        }
    } 
    if(hashtable[4] != 32) 
    {
        if ((hashtable[0] == hashtable[4]) && (hashtable[0] == hashtable[8]) || 
            (hashtable[2] == hashtable[4]) && (hashtable[2] == hashtable[6]))
        {
            return hashtable[4];
        }
    }
    return 0;
}


// TCP Server: socket[] -> bind[] -> listen[] -> accept[] 
// TCP Client: socket[] -> connect[] -> receive[]
// UDP Client: socket[] -> sendto[]
int main(int argc, char ** argv) 
{
    socklen_t len;
    ssize_t nbytes;
    int client_sockfd, listen_fd, player_fd;
    struct sockaddr_in serv_addr, player2_addr;
    
    char server_message[64];
    struct client_info other_player;

    /*Protocolos: */
    unsigned char CONNECT;
    unsigned char ACK_NACK;
    uint16_t CHANGE_PORT;

    CONNECT = 0;

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

    if (!strncmp("UDP", argv[2], 3) || !strncmp("udp", argv[2], 3))
    {
        if ((client_sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1 )
        {
            printf("Error: socket not created");
            exit(EXIT_FAILURE);
        }
        if (sendto(client_sockfd, (const char *) &CONNECT, sizeof(CONNECT), 0, 
                    (const struct sockaddr *) &serv_addr, sizeof(serv_addr)) == -1)
        {
            printf("Error: sendto failed!");
            exit(EXIT_FAILURE);
        }
        printf("    enviou CONNECT %d\n", CONNECT);
        len = sizeof(serv_addr);
        if (recvfrom(client_sockfd, &ACK_NACK, sizeof(ACK_NACK), 0, 
                        (struct sockaddr *) &serv_addr, &len) == -1)
        {
            printf("Error: recvfrom failed!");
            exit(EXIT_FAILURE);
        }
        if (ACK_NACK == 1)
        {
            printf("    Alcançou o servidor.\n");
        }
        if (recvfrom(client_sockfd, (void*) &CHANGE_PORT, sizeof(CHANGE_PORT), 0, 
                        (struct sockaddr *) &serv_addr, &len) == -1)
        {
            printf("Error: recvfrom failed!");
            exit(EXIT_FAILURE);
        }
        if (CHANGE_PORT != 0)
        {
            close(client_sockfd);
            printf("Aux port: %d\n", ntohs(CHANGE_PORT));
            bzero(&serv_addr, sizeof(serv_addr));
            serv_addr.sin_family = AF_INET;
            serv_addr.sin_port = CHANGE_PORT;
            serv_addr.sin_addr.s_addr = inet_addr("192.168.15.15");
            printf("Aux port: %d\n", ntohs(serv_addr.sin_port));
            if ((client_sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1 )
            {
                printf("Error: socket not created");
                exit(EXIT_FAILURE);
            }
        }
    }
    else
    {
        if ((client_sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1 )
        {
            printf("Error: socket not created");
            exit(EXIT_FAILURE);
        }
        if ((connect(client_sockfd, (const struct sockaddr *) &serv_addr,
                        sizeof(serv_addr))) == -1)
        {
            printf("Error: connect failed");
            exit(EXIT_FAILURE);
        }
    }

    struct pollfd poll_fd[1];
    int ret, t_out;
    t_out = 1000;

    struct client_info player1;

    /*****************************************************************************************************/
    /*    FRONT_END LOOP                                                                                 */
    /*****************************************************************************************************/
    while (1)
    {
        poll_fd[0].fd = client_sockfd;
        poll_fd[0].events = POLLIN;

        ret = poll(poll_fd, 1,  1000);
        if (ret == -1)
        {
            printf("Error: poll from handler failed");
            exit(EXIT_FAILURE);
        }
        if (!game_on && (poll_fd[0].revents != 0) && (poll_fd[0].fd == client_sockfd))
        {
            if (!strncmp("UDP", argv[2], 3) || !strncmp("udp", argv[2], 3))
            {
                len = sizeof(serv_addr);
                if (recvfrom(client_sockfd, server_message, sizeof(server_message), 0, 
                                        (struct sockaddr *) &serv_addr, &len) == -1)
                {
                    printf("Error: recvfrom failed!");
                    exit(EXIT_FAILURE);
                }
            }
            else
            {
                if (recv(client_sockfd, server_message, sizeof(server_message), 0) == -1)
                {
                    printf("Error: recv failed!");
                    exit(EXIT_FAILURE);
                }
            }
            if (!strncmp(server_message, "ping", 4))
            {
                /* code */
            }
            else if (!strncmp(server_message, "game", 4))
            {
                char accept;
                printf("    Aceitar?(y/n): ");
                scanf("%c", &accept);
                getchar();
                if (accept == 121)
                {
                    if (!strncmp("UDP", argv[2], 3) || !strncmp("udp", argv[2], 3))
                    {
                        len = sizeof(serv_addr);
                        if (recvfrom(client_sockfd,(void *) &player1, sizeof(struct client_info), 0, 
                                                (struct sockaddr *) &serv_addr, &len) == -1)
                        {
                            printf("Error: recvfrom failed!");
                            exit(EXIT_FAILURE);
                        }
                    }
                    else
                    {
                        if (recv(client_sockfd,(void *) &player1, sizeof(struct client_info), 0) == -1)
                        {
                            printf("Error: recv failed!");
                            exit(EXIT_FAILURE);
                        }
                    }
                    printf("Dados do player 1 recebidos!\n");
                    struct sockaddr_in player1_addr;
                    bzero(&player1_addr, sizeof(player1_addr));
                    player1_addr.sin_family = AF_INET;
                    player1_addr.sin_port = htons(player1.P2P_port);
                    player1_addr.sin_addr.s_addr = inet_addr(player1.ip); 

                    if ((player_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1 )
                    {
                        printf("Error: socket not created");
                        exit(EXIT_FAILURE);
                    }
                    if ((connect(player_fd, (const struct sockaddr *) &player1_addr,
                                    sizeof(player1_addr))) == -1)
                    {
                        printf("Error: connect failed");
                        exit(EXIT_FAILURE);
                    }
                    printf("Conectado ao player 1!\n");
                    ACK_NACK = 1;
                    if(send(player_fd, (void*) &ACK_NACK, sizeof(ACK_NACK), 0) == -1)
                    {
                        printf("Error: send to other player failed");
                        exit(EXIT_FAILURE);
                    }
                    sprintf(user_input, "playing");
                    if (!strncmp("UDP", argv[2], 3) || !strncmp("udp", argv[2], 3))
                    {
                        if (sendto(client_sockfd, (void *) user_input, strlen(user_input), 0, 
                                    (const struct sockaddr *) &serv_addr, sizeof(serv_addr)) == -1)
                        {
                            printf("Error: sendto failed!");
                            exit(EXIT_FAILURE);
                        }
                    }
                    is_player1 = false;
                    game_on = true;
                    hashtable = (unsigned char *) malloc(sizeof(unsigned char)*9);
                    for (int i = 0; i < 9; i++) hashtable[i] = 32; 
                    print_hash_table(hashtable);
                    if (recv(player_fd, (void *) hashtable, sizeof(char)*9, 0) == -1) 
                    {
                        printf("Error: receive failed");
                        exit(EXIT_FAILURE);
                    }
                    tie--;
                    print_hash_table(hashtable);
                } else {
                    ACK_NACK = 0;
                    if(send(player_fd, (void*) &ACK_NACK, sizeof(ACK_NACK), 0) == -1)
                    {
                        printf("Error: send to other player failed");
                        exit(EXIT_FAILURE);
                    }
                }
            }
            
            
        }
        else if(ret == 0)
        {
            printf("JogoDaVelha> ");
            fgets(user_input, 64, stdin);
            user_input[strlen(user_input) - 1] = '\0'; 
            printf("%s\n", user_input);
            strncpy(user_input_copy, user_input, strlen(user_input));
            user_input_copy[strlen(user_input)] = '\0';
            command = strtok(user_input_copy, " ");
            /* NEW *---------------------------------------------------------------------------------*/
            if (!strncmp(command, "new", 3)) 
            {
                if (logged)
                {
                    printf("    Já está logado.\n");
                    continue;
                }
                if (game_on)
                {
                    printf("    Você está jogando.\n");
                    continue;
                } 
                command = strtok(NULL, " "); 
                username = (char *) malloc(sizeof(strlen(command)));
                strncpy(username, command, strlen(command));
                username[strlen(username)] = '\0';           
                if (!strncmp("UDP", argv[2], 3) || !strncmp("udp", argv[2], 3))
                {
                    if (sendto(client_sockfd, (void *) user_input, strlen(user_input), 0, 
                                (const struct sockaddr *) &serv_addr, sizeof(serv_addr)) == -1)
                    {
                        printf("Error: sendto failed!\n");
                        exit(EXIT_FAILURE);
                    }
                    len = sizeof(serv_addr);
                    if (recvfrom(client_sockfd, (void *) &ACK_NACK, sizeof(ACK_NACK), 0, 
                                    (struct sockaddr *) &serv_addr, &len) == -1)
                    {
                        printf("Error: recvfrom failed!\n");
                        exit(EXIT_FAILURE);
                    } 
                    printf("ACK_NACK recebido!\n");
                    if (ACK_NACK == 1)
                    {
                        printf("    Usuário criado.\n");
                    }
                    else
                    {
                        printf("    Usuário não criado.\n");
                    }
                } 
                else
                {
                    if (send(client_sockfd, (void *) user_input, strlen(user_input), 0) == -1)
                    {
                        printf("Error: send 1 failed!");
                        exit(EXIT_FAILURE);
                    }
                    if (recv(client_sockfd, (void *) &ACK_NACK, sizeof(ACK_NACK), 0) == -1)
                    {
                        printf("Error: recv 1 failed!");
                        exit(EXIT_FAILURE);
                    }
                    if (ACK_NACK == 1)
                    {
                        printf("    Usuário criado.\n");
                    }
                    else
                    {
                        printf("    Usuário não criado.\n");
                    }
                }
            }
            /* PASS *---------------------------------------------------------------------------------*/
            else if (!strncmp(command, "pass", 4))
            {
                if (!logged)
                {
                    printf("    Não está logado.\n");
                    continue;
                }
                if (game_on)
                {
                    printf("    Você está jogando.\n");
                    continue;
                } 
                if (!strncmp("UDP", argv[2], 3) || !strncmp("udp", argv[2], 3))
                {
                    if (sendto(client_sockfd, (const char *) user_input, strlen(user_input), 0, 
                                    (const struct sockaddr *) &serv_addr, sizeof(serv_addr)) == -1)
                    {
                        printf("Error: sendto failed!");
                        exit(EXIT_FAILURE);
                    }
                    if (recvfrom(client_sockfd, &ACK_NACK, sizeof(ACK_NACK), 0, 
                                    (struct sockaddr *) &serv_addr, &len) == -1)
                    {
                        printf("Error: recvfrom failed!");
                        exit(EXIT_FAILURE);
                    } 
                    if (ACK_NACK == 1)
                    {
                        printf("    Password mudado\n");
                    }
                }
                else 
                {
                    if (send(client_sockfd, (const char *) user_input, strlen(user_input), 0) == -1)
                    {
                        printf("Error: send failed!");
                        exit(EXIT_FAILURE);
                    }
                    if (recv(client_sockfd, &ACK_NACK, sizeof(ACK_NACK), 0) == -1)
                    {
                        printf("Error: recv 1 failed!");
                        exit(EXIT_FAILURE);
                    }
                    if (ACK_NACK == 1)
                    {
                        printf("    Password mudado\n");
                    }
                }
            }
            /* IN *-----------------------------------------------------------------------------------*/
            else if (!strncmp(command, "in", 3))
            {
                if (logged)
                {
                    printf("    Já está logado\n");
                    continue;
                }
                if (game_on)
                {
                    printf("    Você está jogando.\n");
                    continue;
                } 

                if (!strncmp("UDP", argv[2], 3) || !strncmp("udp", argv[2], 3))
                {
                    if (sendto(client_sockfd, (void *) user_input, strlen(user_input), 0, 
                                (const struct sockaddr *) &serv_addr, sizeof(serv_addr)) == -1)
                    {
                        printf("Error: sendto failed!");
                        exit(EXIT_FAILURE);
                    }
                    len = sizeof(serv_addr);
                    if ((nbytes = recvfrom(client_sockfd, (void *) &ACK_NACK, sizeof(ACK_NACK), 0, 
                                    (struct sockaddr *) &serv_addr, &len)) == -1)
                    {
                        printf("Error: recvfrom failed!");
                        exit(EXIT_FAILURE);
                    } 
                    if (ACK_NACK == 1)
                    {
                        logged = true;
                        printf("    Você está logado\n");
                    }
                    if (ACK_NACK == 0)
                    {
                        printf("    Usuário ou senha incorretos.\n");
                    }
                } 
                else
                {
                    if (send(client_sockfd, (const void *) user_input, strlen(user_input), 0) == -1)
                    {
                        printf("Error: send 1 failed!");
                        exit(EXIT_FAILURE);
                    }
                    if (recv(client_sockfd, &ACK_NACK, sizeof(ACK_NACK), 0) == -1)
                    {
                        printf("Error: recv 1 failed!");
                        exit(EXIT_FAILURE);
                    }
                    if (ACK_NACK == 1)
                    {
                        logged = true;
                        printf("    Você está logado\n");
                    }
                    if (ACK_NACK == 0)
                    {
                        printf("    Usuário ou senha incorretos.\n");
                    }
                }
            }
            /* HALLOFFAME *---------------------------------------------------------------------------*/
            else if (!strncmp(command, "halloffame", 10))
            {
                if (!logged)
                {
                    printf("    Não está logado\n");
                    continue;
                }
                if (!strncmp("UDP", argv[2], 3) || !strncmp("udp", argv[2], 3))
                {
                    if (sendto(client_sockfd, (const char *) user_input, strlen(user_input), 0, 
                                    (const struct sockaddr *) &serv_addr, sizeof(serv_addr)) == -1)
                    {
                        printf("Error: sendto failed!");
                        exit(EXIT_FAILURE);
                    }
                    printf("\n*** Hall of Fame ***\n\n");
                    server_message[0] = 1;
                    while(server_message[0] != 0)
                    {
                        memset((void *)server_message, 0, 64);
                        if (recvfrom(client_sockfd, (void*) server_message, sizeof(server_message), 0, 
                                    (struct sockaddr *) &serv_addr, &len) == -1)
                        {
                            printf("Error: recvfrom failed!");
                            exit(EXIT_FAILURE);
                        } 
                        printf("    %s\n", server_message);
                    }
                }
                else 
                {
                    if (send(client_sockfd, (const char *) user_input, strlen(user_input), 0) == -1)
                    {
                        printf("Error: send failed!");
                        exit(EXIT_FAILURE);
                    }
                    printf("\n*** Hall of Fame ***\n\n");
                    server_message[0] = 1;
                    while(server_message[0] != 0)
                    {
                        memset((void *)server_message, 0, 64);
                        if (recv(client_sockfd, (void*) server_message, sizeof(server_message), 0) == -1)
                        {
                            printf("Error: recv failed!");
                            exit(EXIT_FAILURE);
                        } 
                        printf("    %s\n", server_message);
                    }
                }
            }
            /* L *------------------------------------------------------------------------------------*/
            else if (!strncmp(command, "l", 1))
            {
                if (!logged)
                {
                    printf("    Não está logado\n");
                    continue;
                }
                if (!strncmp("UDP", argv[2], 3) || !strncmp("udp", argv[2], 3))
                {
                    if (sendto(client_sockfd, (const char *) user_input, strlen(user_input), 0, 
                                    (const struct sockaddr *) &serv_addr, sizeof(serv_addr)) == -1)
                    {
                        printf("Error: sendto failed!");
                        exit(EXIT_FAILURE);
                    }
                    printf("\nUsuário Online\n");
                    printf("(usuário | jogando)\n");
                    server_message[0] = 1;
                    while(server_message[0] != 0)
                    {
                        memset((void *)server_message, 0, 64);
                        len = sizeof(serv_addr);
                        if (recvfrom(client_sockfd, server_message, sizeof(server_message), 0, 
                                    (struct sockaddr *) &serv_addr, &len) == -1)
                        {
                            printf("Error: recvfrom failed!");
                            exit(EXIT_FAILURE);
                        } 
                        printf("    %s\n", server_message);
                    }
                }
                else 
                {
                    if (send(client_sockfd, (const char *) user_input, strlen(user_input), 0) == -1)
                    {
                        printf("Error: send failed!");
                        exit(EXIT_FAILURE);
                    }
                    printf("\nUsuário Online\n");
                    printf("(usuário | jogando)\n");;
                    server_message[0] = 1;
                    while(server_message[0] != 0)
                    {
                        memset((void *)server_message, 0, 64);
                        if (recv(client_sockfd, &server_message, sizeof(server_message), 0) == -1)
                        {
                            printf("Error: recv failed!");
                            exit(EXIT_FAILURE);
                        } 
                        printf("    %s\n", server_message);
                    }
                }
            }
            /* CALL *---------------------------------------------------------------------------------*/
            else if (!strncmp(command, "call", 4))
            {
                if (!logged)
                {
                    printf("    Não está logado\n");
                    continue;
                }
                if (game_on)
                {
                    printf("    Você está jogando.\n");
                    continue;
                } 
                if (!strncmp("UDP", argv[2], 3) || !strncmp("udp", argv[2], 3))
                {
                    if (sendto(client_sockfd, (const char *) user_input, strlen(user_input), 0, 
                                    (const struct sockaddr *) &serv_addr, sizeof(serv_addr)) == -1)
                    {
                        printf("Error: sendto failed!");
                        exit(EXIT_FAILURE);
                    }
                    if (recvfrom(client_sockfd, &ACK_NACK, sizeof(unsigned char), 0, 
                                    (struct sockaddr *) &serv_addr, &len) == -1)
                    {
                        printf("Error: recvfrom failed!");
                        exit(EXIT_FAILURE);
                    } 
                    if (ACK_NACK)
                    {
                        if (recvfrom(client_sockfd, &other_player, sizeof(other_player), 0, 
                                    (struct sockaddr *) &serv_addr, &len) == -1)
                        {
                            printf("Error: recvfrom failed!");
                            exit(EXIT_FAILURE);
                        }
                    }
                    else
                    {
                        printf("    Jogador não está online!\n");
                    }
                }
                else 
                {
                    if (send(client_sockfd, (const char *) user_input, strlen(user_input), 0) == -1)
                    {
                        printf("Error: send failed!");
                        exit(EXIT_FAILURE);
                    }
                    if (recv(client_sockfd, &ACK_NACK, sizeof(ACK_NACK), 0) == -1)
                    {
                        printf("Error: recv failed!");
                        exit(EXIT_FAILURE);
                    }
                    if (ACK_NACK)
                    {
                        if (recv(client_sockfd, &other_player, sizeof(other_player), 0) == -1)
                        {
                            printf("Error: recv failed!");
                            exit(EXIT_FAILURE);
                        }  
                    }
                    else
                    {
                        printf("    Jogador inexistente ou não está online!\n");
                    }    
                }
                if (ACK_NACK)
                {
                    struct sockaddr_in player2_addr;
                    memset((void *)&player2_addr, 0, sizeof(player2_addr));
                    player2_addr.sin_family = AF_INET;
                    player2_addr.sin_port = htons(other_player.P2P_port);
                    player2_addr.sin_addr.s_addr = htonl(INADDR_ANY);

                    if ((listen_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1 )
                    {
                        printf("Error: socket not created");
                        exit(EXIT_FAILURE);
                    }
                    if (bind(listen_fd, (struct sockaddr *) &player2_addr, sizeof(player2_addr)) == -1)
                    {
                        printf("Error: listen_fd bind failed");
                        exit(EXIT_FAILURE);
                    }
                    if ((listen(listen_fd, 10)) == -1)
                    {
                        printf("Error: listen failed");
                        exit(EXIT_FAILURE);
                    }
                    printf("player 1 listening on port: %d\n", ntohs(player2_addr.sin_port));
                    len = sizeof(player2_addr);
                    if ((player_fd = accept(listen_fd, (struct sockaddr*)&player2_addr, &len)) == -1)
                    {
                        printf("Error: accept failed");
                        exit(EXIT_FAILURE);
                    }
                    if (recv(player_fd, &ACK_NACK, sizeof(ACK_NACK), 0) == -1)
                    {
                        printf("Error: recv 1 failed!");
                        exit(EXIT_FAILURE);
                    } 
                    if (ACK_NACK == 1)
                    {
                        printf("    Convite aceito!\n");
                        char call[14] = "call accepted";
                        if (!strncmp("UDP", argv[2], 3) || !strncmp("udp", argv[2], 3))
                        {
                            if (sendto(client_sockfd, (const char *) call, strlen(call), 0, 
                                            (const struct sockaddr *) &serv_addr, sizeof(serv_addr)) == -1)
                            {
                                printf("Error: sendto failed!");
                                exit(EXIT_FAILURE);
                            }
                            game_on = true;
                        }
                        else 
                        {
                            if (send(client_sockfd, (const char *) call, strlen(call), 0) == -1)
                            {
                                printf("Error: send failed!");
                                exit(EXIT_FAILURE);
                            }
                        } 
                        game_on = true;
                        is_player1 = true;
                        hashtable = (unsigned char *) malloc(sizeof(unsigned char)*9);
                        for (int i = 0; i < 9; i++) hashtable[i] = 32; 
                    }
                    else
                    {
                        printf("    Convite negado");
                    }
                }
            }
            /* PLAY *---------------------------------------------------------------------------------*/
            else if (!strncmp(command, "play", 4))
            {
                if (!logged)
                {
                    printf("    Não está logado\n");
                    continue;
                }
                if (!game_on)
                {
                    printf("    Não está jogando\n");
                    continue;
                }
                line = atoi(strtok(NULL, " "));
                column = atoi(strtok(NULL, " "));
                print_hash_table(hashtable); 
                if (game_end == 0 && tie > 0) 
                {
                    if (is_player1) hashtable = hash_game(hashtable, 88, line, column);
                    else hashtable = hash_game(hashtable, 79, line, column);
                    if (send(player_fd, (void *) hashtable, sizeof(char)*9, 0) == -1) 
                    {
                        printf("Error: sending failed");
                        exit(EXIT_FAILURE);
                    }
                    tie--;
                    print_hash_table(hashtable);
                    game_end = hash_winner(hashtable);
                    printf("game_end: %d tie:  %d", game_end, tie);
                    if (game_end == 0 && tie > 0)
                    {
                        if (recv(player_fd, (void *) hashtable, sizeof(char)*9, 0) == -1) 
                        {
                            printf("Error: receive failed");
                            exit(EXIT_FAILURE);
                        }
                        if (hashtable[0] == 0)
                        {
                            sleep(3);
                            char * over = "over";
                            if (!strncmp("UDP", argv[2], 3) || !strncmp("udp", argv[2], 3))
                            {
                                if (sendto(client_sockfd, (const void *) over, strlen(over), 0, 
                                                (const struct sockaddr *) &serv_addr, sizeof(serv_addr)) == -1)
                                {
                                    printf("Error: sendto failed!");
                                    exit(EXIT_FAILURE);
                                }
                            }
                            else
                            {
                                if (send(client_sockfd, (const void *) over, strlen(over), 0) == -1)
                                {
                                    printf("Error: send 1 failed!");
                                    exit(EXIT_FAILURE);
                                }
                                
                            }
                            continue;
                        }
                        print_hash_table(hashtable);
                        game_end = hash_winner(hashtable);
                    }
                } 
                if (is_player1)
                {
                    if ( game_end || tie < 0)
                    {
                        if (game_end == 88) 
                        {
                            printf("\n\nVocê Venceu!\n\n"); 
                            sprintf(user_input, "game_end");
                            if (!strncmp("UDP", argv[2], 3) || !strncmp("udp", argv[2], 3))
                            {
                                if (sendto(client_sockfd, (const char *) user_input, strlen(user_input), 0, 
                                        (const struct sockaddr *) &serv_addr, sizeof(serv_addr)) == -1)
                                {
                                    printf("Error: sendto failed!");
                                    exit(EXIT_FAILURE);
                                }
                            } 
                            else 
                            {
                                if (send(client_sockfd, (const char *) &user_input, strlen(user_input), 0) == -1)
                                {
                                    printf("Error: send failed!");
                                    exit(EXIT_FAILURE);
                                }
                            }
                        
                        }
                        if (game_end == 79) printf("\n\nVocê Perdeu!\n\n");
                        if (!game_end && tie < 0) printf("\n\nEmpate!\n\n");
                        game_on = false;
                        game_end = 0;
                        tie = 5;
                        free(hashtable);
                    }   
                }
                else
                {
                    if (game_end || tie < 1)
                    {
                        if (game_end == 79) 
                        {
                            printf("\n\nVocê Venceu!\n\n");
                            sprintf(user_input, "game_end %d", game_end);
                            user_input[11] = '\0';
                            if (!strncmp("UDP", argv[2], 3) || !strncmp("udp", argv[2], 3))
                            {
                                if (sendto(client_sockfd, (const char *) user_input, strlen(user_input), 0, 
                                        (const struct sockaddr *) &serv_addr, sizeof(serv_addr)) == -1)
                                {
                                    printf("Error: sendto failed!");
                                    exit(EXIT_FAILURE);
                                }
                            } 
                            else 
                            {
                                if (send(client_sockfd, (const char *) &user_input, strlen(user_input), 0) == -1)
                                {
                                    printf("Error: send failed!");
                                    exit(EXIT_FAILURE);
                                }
                            }
                        }
                        if (game_end == 88) printf("\n\nVocê Perdeu!\n\n");
                        if (!game_end && tie < 1) printf("\n\nEmpate!\n\n");
                        game_on = false;
                        game_end = 0;
                        tie = 5;
                        free(hashtable);
                    }
                }
            }
            /* DELAY *--------------------------------------------------------------------------------*/
            else if (!strncmp(command, "delay", 5))
            {
                if (!logged)
                {
                    printf("    Não está logado\n");
                    continue;
                }
            }
            /* OVER *---------------------------------------------------------------------------------*/
            else if (!strncmp(command, "over", 4))
            {
                if (!logged)
                {
                    printf("    Não está logado\n");
                    continue;
                }
                if (!game_on)
                {
                    printf("    Não está jogando.\n");
                    continue;
                }
                ACK_NACK = 0;
                if (send(player_fd, (void *) &ACK_NACK, sizeof(unsigned char), 0) == -1) 
                {
                    printf("Error: sending failed");
                    exit(EXIT_FAILURE);
                }
                if (!strncmp("UDP", argv[2], 3) || !strncmp("udp", argv[2], 3))
                {
                    if (sendto(client_sockfd, (const char *) user_input, strlen(user_input), 0, 
                                    (const struct sockaddr *) &serv_addr, sizeof(serv_addr)) == -1)
                    {
                        printf("Error: sendto failed!");
                        exit(EXIT_FAILURE);
                    }
                }
                else
                {
                    if (send(client_sockfd, (const void *) user_input, strlen(user_input), 0) == -1)
                    {
                        printf("Error: send 1 failed!");
                        exit(EXIT_FAILURE);
                    }
                }
                game_on = false;
                close(player_fd);
                free(hashtable);
            }
            /* OUT *----------------------------------------------------------------------------------*/
            else if (!strncmp(command, "out", 3))
            {
                if (!logged)
                {
                    printf("    Não está logado\n");
                    continue;
                }
                if (game_on)
                {
                    printf("    Você está jogando.\n");
                    continue;
                } 
                if (!strncmp("UDP", argv[2], 3) || !strncmp("udp", argv[2], 3))
                {
                    if (sendto(client_sockfd, (const char *) user_input, strlen(user_input), 0, 
                                    (const struct sockaddr *) &serv_addr, sizeof(serv_addr)) == -1)
                    {
                        printf("Error: sendto failed!");
                        exit(EXIT_FAILURE);
                    }
                    if (recvfrom(client_sockfd, &ACK_NACK, sizeof(ACK_NACK), 0, 
                                    (struct sockaddr *) &serv_addr, &len) == -1)
                    {
                        printf("Error: recvfrom failed!");
                        exit(EXIT_FAILURE);
                    } 
                    if (ACK_NACK == 1)
                    {
                        logged = false;
                        printf("    deslogado.\n");
                    }
                    if (ACK_NACK == 0)
                    {
                        printf("    ainda permanece logado.\n");
                    }
                }
                else
                {
                    if (send(client_sockfd, (const void *) user_input, strlen(user_input), 0) == -1)
                    {
                        printf("Error: send 1 failed!");
                        exit(EXIT_FAILURE);
                    }
                    if (recv(client_sockfd, &ACK_NACK, sizeof(ACK_NACK), 0) == -1)
                    {
                        printf("Error: recv 1 failed!");
                        exit(EXIT_FAILURE);
                    }
                    if (ACK_NACK == 1)
                    {
                        logged = false;
                        printf("    deslogado.\n");
                    }
                    if (ACK_NACK == 0)
                    {
                        printf("    ainda permanece logado.\n");
                    }
                }
            }
            /* BYE *----------------------------------------------------------------------------------*/
            else if (!strncmp(command, "bye", 3))
            {
                return 0;
            }
            
            else
            {
                printf("Comando desconhecido!\n");
            }
        }
    }
}

    // if (!strncmp("UDP", argv[2], 3) || !strncmp("udp", argv[2], 3))
    // {
    //     if ((client_sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1 )
    //     {
    //         printf("Error: socket not created");
    //         exit(EXIT_FAILURE);
    //     }
    //     if (sendto(client_sockfd, (const char *) CONNECT, strlen(CONNECT), 0, 
    //                  (const struct sockaddr *) &serv_addr, sizeof(serv_addr)) == -1)
    //     {
    //         printf("Error: sendto failed!");
    //         exit(EXIT_FAILURE);
    //     }
    //     len = sizeof(serv_addr);
    //     if ((nbytes = recvfrom(client_sockfd, server_message, sizeof(server_message), 0, 
    //                     (struct sockaddr *) &serv_addr, &len)) == -1)
    //     {
    //         printf("Error: recvfrom failed!");
    //         exit(EXIT_FAILURE);
    //     }
    //     printf("    Server response: %s\n", server_message);
    //     if((nbytes = recvfrom(client_sockfd, (void *) &other_player, sizeof(other_player), 0, 
    //                 (struct sockaddr *) &serv_addr, (socklen_t *) &len)) == -1)
    //     {
    //         printf("Error: sendto failed!");
    //         exit(EXIT_FAILURE);
    //     }
    // } 
    // else
    // {
    //     if ((client_sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1 )
    //     {
    //         printf("Error: socket not created");
    //         exit(EXIT_FAILURE);
    //     }
    //     if ((connect(client_sockfd, (const struct sockaddr *) &serv_addr,
    //                     sizeof(serv_addr))) == -1)
    //     {
    //         printf("Error: connect failed");
    //         exit(EXIT_FAILURE);
    //     }
    //     if (send(client_sockfd, (const void *) CONNECT, strlen(CONNECT), 0) == -1)
    //     {
    //         printf("Error: send 1 failed!");
    //         exit(EXIT_FAILURE);
    //     }
    //     if (recv(client_sockfd, server_message, sizeof(server_message), 0) == -1)
    //     {
    //         printf("Error: recv 1 failed!");
    //         exit(EXIT_FAILURE);
    //     }
    //     printf("    Server response: %s\n", server_message);
    //     if(recv(client_sockfd, (void *) &other_player, sizeof(other_player), 0) == -1)
    //     {
    //         printf("Error: recv failed!");
    //         exit(EXIT_FAILURE);
    //     }
    // }
    
//     printf(" Player info: \n");
//     printf("          ip: %s\n", other_player.ip);
//     printf("        port: %d\n", other_player.port);
//     printf("    P2P_port: %d\n", other_player.P2P_port);
//     printf("        main: %d\n", other_player.main);

//     unsigned char game_end;
//     game_end = 0;
//     int line, column;
//     int tie = 5;
//     unsigned char * hashtable = (unsigned char *) malloc(sizeof(unsigned char)*9);
//     for (int i = 0; i < 9; i++)
//     {
//         hashtable[i] = 32; 
//     } 

//     if (!other_player.main)
//     {
//         struct sockaddr_in player2_addr;
//         bzero(&player2_addr, sizeof(player2_addr));
//         player2_addr.sin_family = AF_INET;
//         player2_addr.sin_port = htons(other_player.P2P_port);
//         player2_addr.sin_addr.s_addr = htonl(INADDR_ANY);

//         if ((listen_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1 )
//         {
//             printf("Error: socket not created");
//             exit(EXIT_FAILURE);
//         }
//         if (bind(listen_fd, (struct sockaddr *) &player2_addr, sizeof(player2_addr)) == -1)
//         {
//             printf("Error: listen_fd bind failed");
//             exit(EXIT_FAILURE);
//         }
//         if ((listen(listen_fd, 10)) == -1)
//         {
//             printf("Error: listen failed");
//             exit(EXIT_FAILURE);
//         }
//         printf("player 1 listening on port: %d\n", ntohs(player2_addr.sin_port));
//         len = sizeof(player2_addr);
//         if ((player_fd = accept(listen_fd, (struct sockaddr*)&player2_addr, &len)) == -1)
//         {
//             printf("Error: accept failed");
//             exit(EXIT_FAILURE);
//         }
        
//         //close(listen_fd);
        
//         print_hash_table(hashtable);
//         while (game_end == 0 && tie > 0)
//         {
//             hashtable = hash_game(hashtable, 88, line, column);
//             if (send(player_fd, (void *) hashtable, sizeof(char)*9, 0) == -1) 
//             {
//                 printf("Error: sending failed");
//                 exit(EXIT_FAILURE);
//             }
//             tie--;
//             print_hash_table(hashtable);
//             game_end = hash_winner(hashtable);
//             printf("game_end: %d tie:  %d", game_end, tie);
//             if (game_end == 0 && tie > 0)
//             {
//                 if (recv(player_fd, (void *) hashtable, sizeof(char)*9, 0) == -1) 
//                 {
//                     printf("Error: receive failed");
//                     exit(EXIT_FAILURE);
//                 }
//                 print_hash_table(hashtable);
//                 game_end = hash_winner(hashtable);
//             }
//         } 
//         if (game_end == 79)
//         {
//             printf("\n\nVocê Venceu!\n\n");
//         }
//         else if(game_end == 88)
//         {
//             printf("\n\nVocê Perdeu!\n\n");
//         }
//         else
//         {
//             printf("\n\nEmpate!\n\n");
//         }
        
//     } 
//     else 
//     {
//         sleep(2);
//         struct sockaddr_in player1_addr;
//         bzero(&player1_addr, sizeof(player1_addr));
//         player1_addr.sin_family = AF_INET;
//         player1_addr.sin_port = htons(other_player.P2P_port);
//         player1_addr.sin_addr.s_addr = inet_addr(other_player.ip); 

//         if ((player_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1 )
//         {
//             printf("Error: socket not created");
//             exit(EXIT_FAILURE);
//         }
//         if ((connect(player_fd, (const struct sockaddr *) &player1_addr,
//                         sizeof(player1_addr))) == -1)
//         {
//             printf("Error: connect failed");
//             exit(EXIT_FAILURE);
//         }

//         print_hash_table(hashtable);
//         while (game_end == 0 && tie > 0)
//         {
//             if (recv(player_fd, (void *) hashtable, sizeof(char)*9, 0) == -1) 
//             {
//                 printf("Error: receive failed");
//                 exit(EXIT_FAILURE);
//             }
//             tie--;
//             print_hash_table(hashtable);
//             game_end = hash_winner(hashtable);
//             printf("game_end: %d tie:  %d", game_end, tie);
//             if (game_end == 0 && tie > 0)
//             {
//                 hashtable = hash_game(hashtable, 79, line, column);  
//                 if (send(player_fd, (void *) hashtable, sizeof(char)*9, 0) == -1) 
//                 {
//                     printf("Error: sending failed");
//                     exit(EXIT_FAILURE);
//                 }
//                 print_hash_table(hashtable);
//                 game_end = hash_winner(hashtable);
//             }
//         } 
//         if (game_end == 79)
//         {
//             printf("\n\nVocê Venceu!\n\n");
//         }
//         else if(game_end == 88)
//         {
//             printf("\n\nVocê Perdeu!\n\n");
//         }
//         else
//         {
//             printf("\n\nEmpate!\n\n");
//         }
//     }
//     close(client_sockfd);
//     return 0;
// }

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
#include <sys/wait.h>
#include <time.h>

#include "C_Aux_Handlers.h"
#include "Hash_Game.h"
#include "Protocol.h"

static bool DEBUG = false;

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
    socklen_t len;
    ssize_t n_bytes;
    int client_sockfd, listen_fd, player_fd;
    uint16_t port, p2p_port;
    bool is_udp;
    char ip_addr[16];

    /*
        Process input terminal initial arguments.
    */ 
    if (argc < 7) {
        printf("usage: ./EP2_Client -p port_number -t protocol -i ip_address\n");
        printf("examples with port = 8000:\n");
        printf("    Protocol TCP:         ./EP2_Client 8000 TCP\n");
        printf("    Protocol UDP:         ./EP2_Client 8000 UDP\n");
        exit(0);
    }
    for (int i = 0; i < argc; i++)
    {
        if (!strncmp(argv[i], "-d", 2) || !strncmp(argv[i], "-D", 2))
        {
            DEBUG = true;
        }
        else if (!strncmp(argv[i], "-p", 2) || !strncmp(argv[i], "-P", 2))
        {
            port = atoi(argv[i + 1]);
            p2p_port = port +1;
            i++;
        }
        else if (!strncmp(argv[i], "-t", 2) || !strncmp(argv[i], "-T", 2))
        {
            /* Checks the protocol and if client connected to server */
            if (!strncmp("UDP", argv[i + 1], 3) || 
                !strncmp("udp", argv[i + 1], 3) || 
                !strncmp("Udp", argv[i + 1], 3)) 
            {
                is_udp = true;
            }
            else if (!strncmp("TCP", argv[i + 1], 3) || 
                    !strncmp("tcp", argv[i + 1], 3) || 
                    !strncmp("Tcp", argv[i + 1], 3)) 
            {
                is_udp = false;
            }
            else
            {
                printf("Protocolo não utilizado.\n");
                exit(EXIT_FAILURE);
            }
            i++; 
        }
        else if (!strncmp(argv[i], "-i", 2) || 
                 !strncmp(argv[i], "-I", 2) ||
                 !strncmp(argv[i], "-ip", 3))
        {
            strncpy(ip_addr,argv[i + 1], strlen(argv[i + 1]));
            ip_addr[strlen(argv[i + 1])] = '\0';
            i++;
        }
    }

    

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
    serv_addr.sin_port = htons(port); 
    serv_addr.sin_addr.s_addr = inet_addr(ip_addr);

    struct sockaddr_in own_addr;

    char username[32], othername[32], l3_delay[64];
    char * command;
    char * user_name;
    char * other_name;
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

    clock_t start, end;
    double delay_time[3];
    for (int i = 0; i < 3; i++) delay_time[i] = 0.0f;
    int c_tick = 0;

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
    unsigned char processed_message[128]; 

    struct pollfd poll_fd[2];
    int ret;

    client_sockfd = Connect_Procedure(is_udp, client_sockfd, &serv_addr, DEBUG);
    if (client_sockfd == -1)
    {
        printf("Error: connection failed!\n");
        exit(EXIT_FAILURE);
    }

    /* 
        Initialize auxiliars processes
    */
    listener  = listener_process(client_sockfd, is_udp, &serv_addr, listener_pipe[1], DEBUG);
    sender    = sender_process(client_sockfd, is_udp, &serv_addr, sender_pipe[0], DEBUG);
    front_end = front_end_process(back_end_pipe[0], front_end_pipe[1], DEBUG); 
    if (listener == 0 || sender == 0 || front_end == 0)
    {
        return 0;
    }

/*  ________________________________________________________________________________________________
    MAIN PROCESS
    ________________________________________________________________________________________________
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


    /*  ________________________________________________________________________________________________
        FRONT_END_PIPE
        Processa mensagem do front-end, return processed_message;
        ________________________________________________________________________________________________
    */
        if ((poll_fd[0].revents == POLLIN) && (poll_fd[0].fd == front_end_pipe[0]))
        {
            read(front_end_pipe[0], (void *) client_message, sizeof(client_message));
            if(DEBUG) printf("[Main process] Main recebeu do front_end: %s\n", client_message);
        /*  BYE  */
            if (!strncmp (client_message, "bye", 3)) 
            {
                printf("Bye command ...exiting!\n");
                write(sender_pipe[1], (void *) client_message, sizeof(client_message));
                // closing pipes
                close(client_sockfd);
                close(listener_pipe[0]); close(listener_pipe[1]);
                close(sender_pipe[0]); close(sender_pipe[1]);
                close(front_end_pipe[0]); close(front_end_pipe[1]);
                close(back_end_pipe[0]); close(back_end_pipe[1]);
                sleep (5);
                // killing aux's
                kill(sender, SIGKILL);
                kill(listener, SIGKILL);
                // front end terminates itself (return 0)
                return 0;
            }
        /*  CALL  */
            else if (!strncmp (client_message, "call", 4))
            {
                kill(front_end, SIGKILL);
                strncpy(processed_message, client_message, sizeof(client_message));
                command = strtok(processed_message, " ");
                other_name  = strtok(NULL, " ");
                strncpy(othername, other_name, strlen(other_name));
                othername[strlen(other_name)] = '\0';
                if(DEBUG) printf("[Main process] othername: %s\n", othername);
                write(sender_pipe[1], (void *) client_message, sizeof(client_message));
                usleep(50000);
            }
        /*  PLAY  */
            else if (!strncmp (client_message, "play", 4))
            {
                command = strtok(client_message, " ");
                line = atoi(strtok(NULL, " "));
                column = atoi(strtok(NULL, " "));
                if (game_end == 0 && tie > 0) 
                {
                    if (is_player1) hashtable = hash_game(hashtable, 'X', line, column);
                    else hashtable = hash_game(hashtable, 'O', line, column);
                    if (send(player_fd, (void *) hashtable, sizeof(char)*9, 0) == -1) 
                    {
                        printf("Error: sending failed");
                        exit(EXIT_FAILURE);
                    }
                    start = clock();
                    tie--;
                    print_hash_table(hashtable);
                    game_end = hash_winner(hashtable);
                    if(DEBUG) printf("[Main process] game_end: %d tie:  %d", game_end, tie);
                    if (game_end == 0 && tie > 0)
                    {
                        kill(front_end, SIGKILL);
                        if (recv(player_fd, (void *) hashtable, sizeof(char)*9, 0) == -1) 
                        {
                            printf("Error: receive failed");
                            exit(EXIT_FAILURE);
                        }
                        end = clock();

                        // saves delay of this turn
                        float aux = ((double) (end - start)*1000) / CLOCKS_PER_SEC;

                        for (int i = 0; i < 3; i++) {
                            float tmp = delay_time[i];
                            delay_time[i] = aux;
                            aux = tmp;
                        }

                        if(DEBUG) printf("Latency: %lf ms\n", delay_time[0]);
                        if (hashtable[0] == 0)
                        {
                            if(DEBUG) printf("[Main process] over hashtable[0] == 0!\n");
                            write(sender_pipe[1], (void *) Game_over, sizeof(Game_over));
                            continue;
                        }
                        print_hash_table(hashtable);
                        game_end = hash_winner(hashtable);
                    }
                } 
                if (is_player1)
                {
                    if (game_end || tie <= 0)
                    {
                        if (game_end == 'X') 
                        {
                            // printf("\n\nVocê Venceu!\n\n"); 
                            memset((void *) processed_message, 0, sizeof(processed_message));
                            sprintf(processed_message, "%s %s %s", I_win, username, othername);
                            if(DEBUG) printf("[Main process] victory processed_message: %s!\n", processed_message);
                            processed_message[strlen(processed_message)] = '\0';
                            write(sender_pipe[1], (void *) processed_message, strlen(processed_message));
                        }
                        // if (game_end == 'O') printf("\n\nVocê Perdeu!\n\n");
                        if (!game_end && tie <= 0) 
                        {
                            // printf("\n\nEmpate!\n\n");
                            memset((void *) processed_message, 0, sizeof(processed_message));
                            sprintf(processed_message, "%s %s %s", Draw, username, othername);
                            if(DEBUG) printf("[Main process] draw processed_message: %s!\n", processed_message);
                            processed_message[strlen(processed_message)] = '\0';
                            write(sender_pipe[1], (void *) processed_message, strlen(processed_message));
                        }
                        game_on = false;
                        game_end = 0;
                        tie = 5;
                        free(hashtable);
                        read(listener_pipe[0], (void *) server_message, sizeof(server_message));
                        server_message[strlen(server_message)] = '\0';
                        printf("    %s\n\n", server_message);
                        is_player1 = false;
                        close(player_fd);
                        p2p_port++;
                    }   
                }
                else
                {
                    if (game_end || tie <= 1)
                    {
                        if (game_end == 'O') 
                        {
                            //printf("\n\nVocê Venceu!\n\n");
                            memset((void *) processed_message, 0, sizeof(processed_message));
                            sprintf(processed_message, "%s %s %s",I_win, username, othername);
                            if(DEBUG) printf("[Main process] victory processed_message: %s!\n", processed_message);
                            processed_message[strlen(processed_message)] = '\0';
                            write(sender_pipe[1], (void *) processed_message, strlen(processed_message));
                        }
                        // if (game_end == 'X') printf("\n\nVocê Perdeu!\n\n");
                        // if (!game_end && tie < 1) printf("\n\nEmpate!\n\n");
                        game_on = false;
                        game_end = 0;
                        tie = 5;
                        free(hashtable);
                        read(listener_pipe[0], (void *) server_message, sizeof(server_message));
                        server_message[strlen(server_message)] = '\0';
                        printf("    %s\n\n", server_message);
                        close(player_fd);
                        p2p_port++;
                    }
                }
                front_end = front_end_process(back_end_pipe[0], front_end_pipe[1], DEBUG); 
                if (front_end == 0)
                {
                    return 0;
                }
            }
        /*  IN  */
            else if (!strncmp (client_message, "in", 2))
            {
                strncpy(processed_message, client_message, sizeof(client_message));
                command = strtok(processed_message, " ");
                user_name  = strtok(NULL, " ");
                strncpy(username, user_name, strlen(user_name));
                username[strlen(user_name)] = '\0';
                if(DEBUG) printf("[Main process] username: %s\n", username);
                write(sender_pipe[1], (void *) client_message, sizeof(client_message));
                usleep(50000);
            }
        /*  DELAY  */
            else if (!strncmp (client_message, "delay", 5))
            {
                sprintf(l3_delay, "Latência (3 últimas): %.3lf ms %.3lf ms %.3lf ms.", delay_time[0], delay_time[1], delay_time[2]);
                write(back_end_pipe[1], (void *) l3_delay, sizeof(l3_delay));
            }
            else 
            {
                write(sender_pipe[1], (void *) client_message, sizeof(client_message));
                usleep(50000);
            }
        }


    /*  ________________________________________________________________________________________________
        LISTENER_PIPE
        Processa mensagem do listener, return processed_message;
        ________________________________________________________________________________________________
    */
        if ((poll_fd[1].revents == POLLIN) && (poll_fd[1].fd == listener_pipe[0]))
        {
            read(listener_pipe[0], (void *) server_message, sizeof(server_message));
            if(DEBUG) printf("[Main process] Main recebeu do main_pipe: %s\n", server_message);
        
        /*  Specials Cases*/
        /*  CALL  */
            if (!strncmp (server_message, "call", 4))
            {
                // memset((void *) processed_message, 0, sizeof(processed_message));
                // strncpy(processed_message, server_message, strlen(server_message));
                server_message[strlen(server_message)] = '\0';
                command = strtok(server_message, " "); 
                user_name = strtok(NULL, " "); 
                other_name = strtok(NULL, " ");
                strncpy(othername, other_name, sizeof(other_name));
                other_ip = strtok(NULL, " ");
                if(DEBUG) printf("[Main process] other_ip: %s\n", other_ip);
                kill(front_end, SIGKILL);
                printf("Invitation for game received from %s!\n", othername);
                printf("    ...accept? (y/n): ");
                scanf("%s", &accept_call);
                getchar();
                if (accept_call == 'y')
                {
                    memset((void *) processed_message, 0, sizeof(processed_message));
                    sprintf(processed_message, "%s %s %s", ACK_accept, user_name, othername);
                    processed_message[strlen(processed_message)] = '\0';
                    if(DEBUG) printf("[Main process] call processed %s len: %zu\n", processed_message,
                                        strlen(processed_message));
                    write(sender_pipe[1], (void *) processed_message, strlen(processed_message));
                    memset((void *)&own_addr, 0, sizeof(own_addr));
                    own_addr.sin_family = AF_INET;
                    own_addr.sin_port = htons(p2p_port);
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
                        if(DEBUG) printf("[Main process] Connect success.\n");
                        if (send(player_fd, (const char *) &CONNECT, sizeof(CONNECT), 0) == -1)
                        {
                            printf("Error: send failed!\n");
                            return -1;
                        }
                        if(DEBUG) printf("[Main process] enviou CONNECT %d\n", CONNECT);
                        if (recv(player_fd, &ACK_NACK, sizeof(ACK_NACK), 0) == -1)
                        {
                            printf("Error: recv failed!\n");
                            return -1;
                        }
                        if (ACK_NACK == 1)
                        {
                            printf("    Alcançou o outro jogador!.\n");
                            hashtable = (unsigned char *) malloc(sizeof(unsigned char)*9);
                            for (int i = 0; i < 9; i++) hashtable[i] = 32;
                            kill(front_end, SIGKILL);
                            print_hash_table(hashtable);
                            if (recv(player_fd, (void *) hashtable, sizeof(char)*9, 0) == -1) 
                            {
                                printf("Error: receive failed");
                                exit(EXIT_FAILURE);
                            }
                            if (hashtable[0] == 0)
                            {
                                if(DEBUG) printf("[Main process] over hashtable[0] == 0!\n");
                                write(sender_pipe[1], (void *) Game_over, sizeof(Game_over));
                                continue;
                            }
                            print_hash_table(hashtable);
                        }
                    }
                    memset((void *) server_message, 0, sizeof(server_message));
                    front_end = front_end_process(back_end_pipe[0], front_end_pipe[1], DEBUG); 
                    if (front_end == 0)
                    {
                        return 0;
                    }
                }
                else
                {
                    memset((void *) server_message, 0, sizeof(server_message));
                    write(sender_pipe[1], (void *) NACK_accept, (size_t) sizeof(NACK_accept));
                } 
            }
        /*  call accepted  */
            else if (!strncmp (server_message, ACK_accept, sizeof(ACK_accept)))
            {
                memset((void *)&own_addr, 0, sizeof(own_addr));
                own_addr.sin_family = AF_INET;
                own_addr.sin_port = htons(p2p_port);
                own_addr.sin_addr.s_addr = htonl(INADDR_ANY);

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
                    if(DEBUG) printf("[Main process] TCP client connected\n");
                    is_player1 = true;
                    hashtable = (unsigned char *) malloc(sizeof(unsigned char)*9);
                    for (int i = 0; i < 9; i++) hashtable[i] = 32;
                    print_hash_table(hashtable);
                    front_end = front_end_process(back_end_pipe[0], front_end_pipe[1], DEBUG); 
                    if (front_end == 0)
                    {
                        return 0;
                    }
                    sleep(1);
                    write(back_end_pipe[1], (void *) ACK_accept, (size_t) sizeof(ACK_accept));
                }
            }
        /*  call rejected  or not online*/
            else if (!strncmp (server_message, NACK_accept, sizeof(NACK_accept)) ||
                     !strncmp (server_message, NACK_accept, sizeof(NACK_accept)))
            {
                printf("    %s\n", NACK_accept);
                front_end = front_end_process(back_end_pipe[0], front_end_pipe[1], DEBUG); 
                if (front_end == 0)
                {
                    return 0;
                }
                sleep(1);
                //write(back_end_pipe[1], (void *) NACK_accept, (size_t) sizeof(NACK_accept));
            }
        /*  Not online*/
            else if (!strncmp (server_message, NACK_online, sizeof(NACK_online)))
            {
                front_end = front_end_process(back_end_pipe[0], front_end_pipe[1], DEBUG); 
                if (front_end == 0)
                {
                    return 0;
                }
                sleep(1);
                write(back_end_pipe[1], (void *) NACK_online, (size_t) sizeof(NACK_online));
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
                kill (listener, SIGKILL);
                kill(sender, SIGKILL);
                kill(front_end, SIGKILL);
                close(client_sockfd);
                if (is_udp)
                {
                    memset((void *)&serv_addr, 0,  sizeof(serv_addr));
                    serv_addr.sin_family = AF_INET;
                    serv_addr.sin_port = htons(port); 
                    serv_addr.sin_addr.s_addr = inet_addr(ip_addr);
                    try_c = 10; // Already wait 30 seconds by ping waiting
                }
                else
                {
                    try_c = 0;
                }
                client_sockfd = -1;
                while (client_sockfd == -1 && try_c < 59)
                {
                    client_sockfd = Connect_Procedure(is_udp, client_sockfd, &serv_addr, DEBUG); 
                    if (!is_udp) sleep(3);
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
                    listener  = listener_process(client_sockfd, is_udp, &serv_addr, listener_pipe[1], DEBUG);
                    sender    = sender_process(client_sockfd, is_udp, &serv_addr, sender_pipe[0], DEBUG);
                    front_end = front_end_process(back_end_pipe[0], front_end_pipe[1], DEBUG); 
                    if (listener == 0 || sender == 0 || front_end == 0)
                    {
                        return 0;
                    } 
                    if (logged)
                    {
                        memset((void *) processed_message, 0, sizeof(processed_message));
                        sprintf(processed_message, "%s %s", NACK_already_logged, username);
                        processed_message[strlen(processed_message)] = '\0';
                        write(sender_pipe[1], (void *) processed_message, strlen(processed_message));
                    }
                }
                printf("...Back to normal with server!\n");
            }
            else
            {
                /*  ACK_in_user  */
                if (!strncmp (server_message, ACK_in_user, sizeof(ACK_in_user)))
                {
                    logged = true;
                }
                else if (!strncmp (server_message, ACK_out_user, sizeof(ACK_out_user)))
                {
                    logged = false;
                }
                write(back_end_pipe[1], (void *) server_message, sizeof(server_message));
                memset((void *) server_message, 0, sizeof(server_message));
                usleep(50000);
            }
        }
    }
} 

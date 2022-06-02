/******************************************************************************
 *  Compilation:  (Use make)
 *  Execution:    ---
 *
 *
 *  DESCRIPTION
 *
 *  Auxiliars Functions for the server to manage the database (database.txt)
 *  and log (log.txt)
 *
 *
 *  PROJECT DECISIONS OR UNFINISHED TASKS (?)
 *
 *  List them bellow
 *
 *  - 
 *
 ******************************************************************************/

#define _GNU_SOURCE         
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>

#include "DB_Manag_Sys.h"

void log_event(char *event) 
{
    FILE* log;
    time_t s = time(NULL);
    struct tm t = *localtime(&s);

    log = fopen("log.txt", "a");
    fprintf(log, "[%d/%02d/%02d - %02d:%02d:%02d]: %s\n",
                t.tm_year + 1900, t.tm_mon + 1, t.tm_mday, 
                t.tm_hour, t.tm_min, t.tm_sec, event);
    fclose(log);
}

int insert_user(char * username, char * password)
{
    FILE * fp;
    if ((fp = fopen("database.txt", "a")) == NULL) {
        printf("Error: database did not opened.\n");
        return 1;
    }
    fprintf(fp,"%s %s %d %d %d %p %d\n", username, password, 0, 0, 0, NULL, 0);
    fclose(fp);
    return 0;
}

int log_user(char * username, char * password, char * new_ip, int new_pipe)
{
    FILE * fp;
    char line[64], line_tokenized[64];
    char *token;
    bool access_denied = true;
    bool check = true;
    if ((fp = fopen("database.txt", "r")) == NULL) {
        printf("Error: database did not opened.\n");
        return 1;
    }
    while (fgets(line, 64, fp) != NULL && access_denied)
    {
        line[strlen(line) - 1] = '\0'; 
        token = strtok(line, " ");
        check = true;
        if (strncmp(token, username, strlen(username) + 1))
        {
            check = false;      
        }
        token = strtok(NULL, " ");
        if (strncmp(token, password, strlen(token) + 1))
        {
            check = false;                 
        }
        if(check) access_denied = false;
    }
    if (access_denied) 
    {
        printf("\nAccess Denied!\n\n");
        return 1;
    }
    else
    {
        change_data(username, 2, new_ip, new_pipe);
        return 0;
    }
    fclose(fp);
}

int change_data(char *username, int cod, char * new_ip, int new_pipe) 
{
    FILE* fp_db; 
    FILE* fp_new_db;
    char *token, *user, *password, *ip;
    int score,  is_on, in_game, pipe_num; 
    in_game = 2;
    char line[64], line_tokenized[64], event[64];
    
    fp_db = fopen("database.txt", "r");
    fp_new_db = fopen("prov_database.txt", "a");
    while (fgets(line, 64, fp_db))
    {
        line[strlen(line) - 1] = '\0'; 
        strncpy(line_tokenized, line, strlen(line));
        token = strtok(line_tokenized, " ");
        if (!strncmp(token, username, strlen(username) + 1))
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
                ip = new_ip;
                pipe_num = new_pipe;

            }
            in_game = atoi(strtok(NULL, " "));
            if (cod == 3) 
            {
                in_game = (in_game == 0) ?  1 :  0;
            }
            if (cod != 2)
            {
                ip = strtok(NULL, " ");
                pipe_num = atoi(strtok(NULL, " "));
            }
            fprintf(fp_new_db,"%s %s %d %d %d %s %d\n", 
                        username, password, score, is_on, in_game, ip, pipe_num);
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
    if (remove("database.txt") != 0)
    {
        printf("Erro: arquivo %s não deletado.\n", "database.txt");
        sprintf(event, "Erro: arquivo %s não deletado.\n", "database.txt");
        log_event(event);
        return 1; 
    }
    if (rename("prov_database.txt", "database.txt") != 0)
    {
        printf("Erro: arquivo %s não atualizado.\n", "database.txt");
        sprintf(event, "Erro: arquivo %s não atualizado.\n", "database.txt");
        log_event(event);
        return 1;
    }
    return 0;  
}

int change_pass(char *username, char *old_pass, char *new_pass) 
{
    FILE* fp_db;
    FILE* fp_new_db;

    char *user, *password, *n_vic, *is_on, *in_game, *ip, *pipe;
    char line[64], line_tokenized[64], event[64];
    int cut;

    fp_db = fopen("database.txt", "r");
    fp_new_db = fopen("prov_database.txt", "a");
    while (fgets(line, 64, fp_db))
    {
        line[strlen(line) - 1] = '\0'; \
        strncpy(line_tokenized, line, strlen(line));\
        user = strtok(line_tokenized, " ");
        if (!strncmp(user, username, strlen(user)))
        {
            password = strtok(NULL, " ");
            if (strncmp(password, old_pass, strlen(old_pass)))
            {
                fclose(fp_db);
                fclose(fp_new_db);
                return 3;
            }
            else  
            {
                n_vic = strtok(NULL, " ");
                is_on = strtok(NULL, " ");
                in_game = strtok(NULL, " ");
                ip = strtok(NULL, " ");
                pipe = strtok(NULL, " ");
                memset((void*) line, 0, 64);
                fprintf(fp_new_db,"%s %s %d %d %d %s %s\n", username, new_pass, 
                            atoi(n_vic), atoi(is_on), atoi(in_game), ip, pipe);

            }  
        }  
        else 
        {
            fprintf(fp_new_db, "%s\n", line);
        }
        memset((void*) line, 0, (size_t) 64);
        memset((void*) line_tokenized, 0, (size_t) 64); 
    } 
    if(remove("database.txt"))
    {
        printf("Erro: arquivo %s não deletado.\n", "database.txt");
        fclose(fp_db);
        fclose(fp_new_db);
        return 1;
    } 
    if(rename("prov_database.txt", "database.txt") == 0)
    {
        sprintf(event, "change_pass: Arquivo %s  atualizado.\n", "database.txt");
        log_event(event);
        fclose(fp_db);
        fclose(fp_new_db);
        return 0;
    } 
    else
    {
        printf("Erro: arquivo %s não atualizado.\n", "database.txt");
        sprintf(event, "Erro: arquivo %s não atualizado.\n", "database.txt");
        log_event(event);
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
                fclose(fp);
                return true;
            } 
            else
            {
                fclose(fp);
                return false;
            }
        }
    }
    fclose(fp);
    return false;
}

int what_pipe(char *username)
{
    FILE *fp;
    char line[64];
    char *token, *n_vic, *password, *is_on, *in_game, *ip; 
    int pipe_num;
    fp = fopen("database.txt", "r");
    while (fgets(line, 64, fp))
    {
        token = strtok(line, " ");
        if (!strncmp(username, token, sizeof(username)))
        {
            password = strtok(NULL, " ");
            n_vic = strtok(NULL, " ");
            is_on = strtok(NULL, " ");
            in_game = strtok(NULL, " ");
            ip = strtok(NULL, " ");
            pipe_num = atoi(strtok(NULL, " "));
            fclose(fp);
            return pipe_num;
        }
    }
    return -1;
}

char * what_ip(char *username)
{
    FILE *fp;
    char line[64];
    char * token, * n_vic, * password, * is_on, * in_game, * ip, * pipe_num;
    char * ip_copy;
    fp = fopen("database.txt", "r");
    while (fgets(line, 64, fp))
    {
        token = strtok(line, " ");
        if (!strncmp(username, token, sizeof(username)))
        {
            password = strtok(NULL, " ");
            n_vic = strtok(NULL, " ");
            is_on = strtok(NULL, " ");
            in_game = strtok(NULL, " ");
            ip = strtok(NULL, " ");
            if (!strncmp(is_on, "1", 1))
            {
                fclose(fp);
                ip_copy = (char *) malloc(strlen(ip));
                strncpy(ip_copy, ip, strlen(ip));
                ip_copy[strlen(ip)] = '\0';
                return ip_copy;
            } 
            else
            {
                fclose(fp);
                return NULL;
            }
        }
    }
    return NULL;
}

int halloffame_sender(int pipe)
{
    FILE * fp;
    char line[64];
    char *token, *score, *user, *pass; 
    if ((fp = fopen("database.txt", "r")) == 0)
    {
        printf("    Banco de dados inexistente.");
        return 1;
    }
    while (fgets(line, 64, fp))
    {
        user = strtok(line, " ");
        pass = strtok(NULL, " ");
        score = strtok(NULL, " ");
        sprintf(line, "%s %s", user, score);
        write(pipe, (void *) line, sizeof(line));
        memset((void *)line, 0, 64);
        usleep(50000);
    }
    fclose(fp);
    usleep(50000);
    return 0;
}

int l_sender(int pipe) 
{
    FILE * fp;
    char line[64];
    char *user, *pass, *n_vic;
    int is_on, in_game;
    if ((fp = fopen("database.txt", "r")) == 0)
    {
        printf("Erro: Banco de dados inexistente.");
        return 1;
    }
    while (fgets(line, 64, fp))
    {
        user = strtok(line, " ");
        pass = strtok(NULL, " ");
        n_vic = strtok(NULL, " ");
        is_on = atoi(strtok(NULL, " "));
        in_game = atoi(strtok(NULL, " "));
        if (is_on)
        {
            if (in_game)
            {
                sprintf(line, "%s | sim", user);
            }
            else
            {
                sprintf(line, "%s | não", user);
            }   
            write(pipe, (void *) line, sizeof(line));
            usleep(50000);
        }
        memset((void *)line, 0, 64);
    }
    fclose(fp);
    usleep(50000);
    return 0;
}

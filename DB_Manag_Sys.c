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
    fprintf(log, "[%d//%02d//%02d - %02d:%02d:%02d]: %s\n",
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
    fprintf(fp,"%s %s %d %d %d\n", username, password, 0, 0, 0);
    fclose(fp);
    return 0;
}

int log_user(char * username, char * password)
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
        printf("token: %s\n", token);
        check = true;
        if (strncmp(token, username, strlen(username) + 1))
        {
            check = false;      
        }
        token = strtok(NULL, " ");
        printf("token: %s\n", token);
        if (strncmp(token, password, strlen(token)))
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
        change_data(username, 2);
        return 0;
    }
    fclose(fp);
}

int change_data(char *username, int cod) 
{
    FILE* fp_db;
    FILE* fp_new_db;

    char *token, *user, *password; 
    int score,  is_on, in_game;
    in_game = 2;
    char line[64], line_tokenized[64];
    
    fp_db = fopen("database.txt", "r");
    fp_new_db = fopen("prov_database.txt", "a");
    while (fgets(line, 64, fp_db))
    {
        line[strlen(line) - 1] = '\0'; 
        printf("change_data: Reading line: %s\n", line);
        strncpy(line_tokenized, line, strlen(line));
        printf("change_data: line_tokenized line: %s\n", line_tokenized);
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
            printf("change_data: New line in DataBase: %s %s %d %d %d\n", username, password, score, is_on, in_game);
            fprintf(fp_new_db,"%s %s %d %d %d\n", username, password, score, is_on, in_game);
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
        return 1; 
    }
    if (rename("prov_database.txt", "database.txt") != 0)
    {
        printf("Erro: arquivo %s não atualizado.\n", "database.txt");
        return 1;
    }
    return 0;  
}

int change_pass(char *username, char *old_pass, char *new_pass) 
{
    FILE* fp_db;
    FILE* fp_new_db;

    char *user, *password, *n_vic, *is_on, *in_game;
    char line[64], line_tokenized[64];
    int cut;

    fp_db = fopen("database.txt", "r");
    fp_new_db = fopen("prov_database.txt", "a");
    while (fgets(line, 64, fp_db))
    {
        line[strlen(line) - 1] = '\0'; 
        printf("change_pass: Reading line: %s\n", line);
        strncpy(line_tokenized, line, strlen(line));
        printf("change_pass: line_tokenized line: %s\n", line_tokenized);
        user = strtok(line_tokenized, " ");
        if (!strncmp(user, username, strlen(user)))
        {
            password = strtok(NULL, " ");
            if (strncmp(password, old_pass, strlen(old_pass)))
            {
                printf("change_pass: Password antigo incorreto!\n");
                fclose(fp_db);
                fclose(fp_new_db);
                return 3;
            }
            else  
            {
                printf("change_pass: Password antigo correto!\n");
                n_vic = strtok(NULL, " ");
                is_on = strtok(NULL, " ");
                in_game = strtok(NULL, " ");
                memset((void*) line, 0, 64);
                printf("change_pass: new line in DataBase: %s %s %d %d %d\n", username, new_pass, atoi(n_vic), atoi(is_on), atoi(in_game));
                fprintf(fp_new_db,"%s %s %d %d %d\n", username, new_pass, atoi(n_vic), atoi(is_on), atoi(in_game));

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
        printf("change_pass: Arquivo %s  atualizado.\n", "database.txt");
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

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include "Hash_Game.h"

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





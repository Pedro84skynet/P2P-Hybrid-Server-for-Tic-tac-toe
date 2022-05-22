#ifndef HASH_GAME_H
#define HASH_GAME_H

unsigned 
char * hash_game(unsigned char * hashtable, unsigned char symbol, 
                            int line, int column);

void print_hash_table(unsigned char * hashtable);

unsigned char hash_winner(unsigned char * hashtable);

#endif

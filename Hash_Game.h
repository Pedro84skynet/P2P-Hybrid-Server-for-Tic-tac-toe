/******************************************************************************
 *  Compilation:  (Use make)
 *  Execution:    ---
 *
 *
 *  DESCRIPTION
 *
 *  Tic-Tac-Toe implemented using an array of chars (very simple implementation)
 *
 *  PROJECT DECISIONS OR UNFINISHED TASKS (?)
 *
 *  List them bellow
 *
 *  - check for bugs 
 *
 ******************************************************************************/

#ifndef HASH_GAME_H
#define HASH_GAME_H


/* main process that runs the game                                            */
unsigned char * hash_game(unsigned char * hashtable, unsigned char symbol, 
                            int line, int column);

/* prints the current game state                                              */       
void print_hash_table(unsigned char * hashtable);

/* calculates if someone won the game in this round                           */
unsigned char hash_winner(unsigned char * hashtable);

#endif

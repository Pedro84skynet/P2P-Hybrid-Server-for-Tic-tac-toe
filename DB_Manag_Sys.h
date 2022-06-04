#ifndef DB_MANAG_SYS_H
#define DB_MANAG_SYS_H

/*    log_event: print a given string in a log.txt file    */
void log_event(char *event);

/*    insert_user: insert a new user in database    */
int insert_user(char * username, char * password);

/*    log_user: log a user in database if password match   */
int log_user(char * username, char * password, char * new_ip, int new_pipe);

/*    change_data: change a boolean stat or a score info in database
      int cod == 1 - Add 3 points in score;
      int cod == 2 - Turn off/on online status info;
      int cod == 3 - Turn off/on "in game" status info;
      int cod == 4 - Add 1 points in score;
      int cod == 4 - updates pipe number;                            */
int change_data(char *username, int cod, char * new_ip, int new_pipe) ; 

/*    change_pass: change a old password from a given username to a new one.    */
int change_pass(char * username, char * old_pass, char * new_pass); 

/*    is_online: returns the info if a given player is online    */
bool is_online(char *username); 

/*    what_pipe: returns the pipe that main allocated for the player    */
int what_pipe(char *username); 

/*    what_pipe: returns the ip of the online player    */
char * what_ip(char *username);

/*    halloffame_sender: returns the list of user's score*/
int halloffame_sender(int pipe);

/*    l_sender: returns the list of user's online an if they are playing*/
int l_sender(int pipe);

#endif
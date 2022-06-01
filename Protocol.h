/******************************************************************************
 *  Compilation:  (Use make)
 *  Execution:    -----
 *
 *
 *  DESCRIPTION
 *
 *  Useful macros shared by multiple files, they will be used in:
 *
 *    - the communication between server and client
 *    - save log of commands use (ask Pedro)
 *    - 
 *
 *  PROJECT DECISIONS OR UNFINISHED TASKS (?)
 *
 *  List them bellow
 *
 *  - 
 *  -
 *
 ******************************************************************************/

#ifndef PROTOCOL_H
#define PROTOCOL_H

#define ACK_new_user         "...new user created!"
#define NACK_new_user        "...new user failed"
#define ACK_in_user          "...logged!"
#define NACK_in_user         "...not logged, username or password incorrect "
#define ACK_newpass_user     "...new password created!"
#define NACK_newpass_user    "...error: new password not created"
#define ACK_out_user         "...logged out!"
#define NACK_out_user        "...error: still logged"
#define ACK_bye_user         "...bye!"
#define NACK_already_logged  "...Already_Logged!"
#define NACK_not_logged      "...you need to be logged!"
#define ACK_hallofame        "********************"
#define NACK_hallofame       "...hall of fame not available"
#define ACK_online_l         "...have fun!"
#define NACK_online_l        "...online list not available"
#define Ping                 "...ping"
#define Reconnect            "...starting reconnection procedure."
#define Server_down          "...lost connection with server."
#define Client_down          "...lost connection with client."
#define ACK_accept           "...accepted!"
#define NACK_accept          "...not accepted!"
#define NACK_online          "...this player is not online!"
#define Game_over            "...rage quit?"
#define Draw                 "...draw!"
#define I_win                "...have_won!"
#define You_won              "...You won! ...congratulations!"
#define You_lose             "...You lose"


#endif


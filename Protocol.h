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

#define ACK_new_user         "...new_user_created!"
#define NACK_new_user        "...new_user_failed"
#define ACK_in_user          "...logged!"
#define NACK_in_user         "...not_logged,_username_or_password_incorrect"
#define ACK_newpass_user     "...new_password_created!"
#define NACK_newpass_user    "...error:_new_password_not_created"
#define ACK_out_user         "...logged_out!"
#define NACK_out_user        "...error:_still_logged"
#define ACK_bye_user         "...bye!"
#define NACK_already_logged  "...Already_Logged!"
#define NACK_not_logged      "...you_need_to_be_logged!"
#define ACK_hallofame        "********************"
#define NACK_hallofame       "...hall_of_fame_not_available"
#define ACK_online_l         "...have_fun!"
#define NACK_online_l        "...online_list_not_available"
#define Ping                 "...ping"
#define Reconnect            "...starting_reconnection_procedure."
#define Server_down          "...lost_connection_with_server."
#define Client_down          "...lost_connection_with_client."
#define ACK_accept           "...accepted!"
#define NACK_accept          "...not_accepted!"
#define NACK_online          "...this_player_is_not_online!"
#define Game_over            "...rage_quit?"
#define Draw                 "...draw!"
#define I_win                "...have_won!"
#define You_won              "...You_won!_...congratulations!"
#define You_lose             "...You_lose"
#define ACK_already_in_game  "...already_in_a_game!"
#define NACK_already_in_game "...not_in_a_game!"

#endif


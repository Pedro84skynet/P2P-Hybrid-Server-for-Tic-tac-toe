CC = gcc
# CFLAGS = -Wall -Wextra -std=c11 -pedantic 
RM = rm

all: EP2_Server EP2_Client

EP2_Server: EP2_Server.o Aux_Handlers.o DB_Manag_Sys.o 
	$(CC) $(CFLAGS) EP2_Server.o Aux_Handlers.o DB_Manag_Sys.o -o EP2_Server

EP2_Client: EP2_Client.o Hash_Game.o 
	$(CC) $(CFLAGS) EP2_Client.o Hash_Game.o -o EP2_Client

EP2_Server.o: EP2_Server.c Aux_Handlers.h DB_Manag_Sys.h
	$(CC) $(CFLAGS) -c EP2_Server.c 

EP2_Client.o: EP2_Client.c Hash_Game.h
	$(CC) $(CFLAGS) -c EP2_Client.c 

Aux_Handlers.o: Aux_Handlers.c Aux_Handlers.h DB_Manag_Sys.h
	$(CC) $(CFLAGS) -c Aux_Handlers.c 

DB_Manag_Sys.o: DB_Manag_Sys.c DB_Manag_Sys.h
	$(CC) $(CFLAGS) -c DB_Manag_Sys.c 

Hash_Game.o: Hash_Game.c Hash_Game.h
	$(CC) $(CFLAGS) -c Hash_Game.c 

clean:
	$(RM) *.o EP2_Server EP2_Client log.txt
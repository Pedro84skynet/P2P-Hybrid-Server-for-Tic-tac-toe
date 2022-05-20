CC = gcc
# CFLAGS = -Wall -pedantic 
RM = rm

all: EP2_Server EP2_Client

EP2_Server: EP2_Server.o  Client_Handler.o
	$(CC) $(CFLAGS) EP2_Server.o Client_Handler.o -o EP2_Server

EP2_Client: EP2_Client.o  Hash_Game.o 
	$(CC) $(CFLAGS) EP2_Client.o Hash_Game.o -o EP2_Client

EP2_Server.o: EP2_Server.c  Client_Handler.h
	$(CC) $(CFLAGS) -c EP2_Server.c 

EP2_Client.o: EP2_Client.c  Hash_Game.h
	$(CC) $(CFLAGS) -c EP2_Client.c 

Client_Handler.o: Client_Handler.c Client_Handler.h
	$(CC) $(CFLAGS) -c Client_Handler.c 

Hash_Game.o: Hash_Game.c Hash_Game.h
	$(CC) $(CFLAGS) -c Hash_Game.c 

clean:
	$(RM) *.o EP2_Server EP2_Client 
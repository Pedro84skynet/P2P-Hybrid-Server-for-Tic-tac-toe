# P2P-Hybrid-Server-for-Tic-tac-toe

Exercise program of discipline MAC0352 (Networks and distributed systems) 
of 2022 of IME USP

MAC0352 - Networks and distributed systems (2022)

The main objective was to create a server that would allow, and manage, client 
tic-tac-toe games playing over P2P TCP. Server should accept both UDP and TCP 
connections and keep a log of events.

1.CONTENTS_____________________________________________________________________


    Source Client:
        EP2_Client.c      
        C_Aux_Handlers.c  
        C_Aux_Handlers.h 
        Hash_Game.c       
        Hash_Game.h  

    Source Server: 
        EP2_Server.c      
        DB_Manag_Sys.c    
        DB_Manag_Sys.h 
        S_Aux_Handlers.c  
        S_Aux_Handlers.h
     
    Protocols: 
        Protocol.h  

    Build Automation Tool:
        Makefile

    Documentation:
        README.md

    License:
        MIT License
   

2.INSTRUCTIONS_________________________________________________________________

    Requirements:

        The machine whose program will be executed must be compatible with
        the main GNU/Linux functions, especially those mentioned by the
        _GNU_SOURCE definition and libraries:

        stdio.h
        stdlib.h
        string.h
        unistd.h
        sys/types.h
        sys/socket.h
        arpa/inet.h
        netinet/in.h
        netinet/in.h
        stdbool.h
        poll.h
        signal.h
        sys/wait.h
        time.h


  2.1   To generate the binary codes, open the unpacked folder in the shell and
        type "make"

          	Exemplo: ~/dir1/dir2 >$ make


  2.2   To run the server binary type:
        "./EP2_Server (port number)"

          	Exemplo: ~/dir1/dir2 >$ ./EP2_Server 5000

        OBS.1: This program uses different ports for each client
        udp. Temporary shutdown of the firewall in a closed environment is 
        recommended. Rules for specific ports are not recommended in this case.

  2.2.1 To run the server binary in debug mode typed:
            "./EP2_Server (port number) -d"

        Exemplo: ~/dir1/dir2 >$ ./EP2_Server 5000 -d

  2.3   To run a client binary type:
        "./EP2_Client -p (port number) -t (protocol type) -i
                                        (Server machine IP address)"

        Exemplo: ~/dir2 >$ ./EP2_Client -p 5000 -t udp -i "192.168.15.138"
        Exemplo: ~/dir2 >$ ./EP2_Client -p 5000 -t tcp -i "192.168.15.138"

  2.3.1 To run a client binary in debug mode type:
        "./EP2_Client -p (port number) -t (protocol type) -i
                                    (Server machine IP address) -d"

        Exemplo: ~/dir2 >$ ./EP2_Client -p 5000 -t udp -i "192.168.15.138" -d

        OBS.2: The client's debug mode makes it difficult to understand and type
        input, but makes visible what is happening behind the front-end,
        like sending and receiving ping.

  2.4   Client commands are the same as those requested in the utterance of 
        this EP:

        • new <username> <password>: create a new user.
                Do not use long names and passwords (> 16 characters).
                Do not use names or passwords with spaces
                    e.g: Pvt. Hudson (Prefer: Pvt_Hudson).
                
        • pass <old password> <new password>: change user password
                Do not use long passwords (> 16 characters).
                
        • in <username> <password>: user enters the server.

        • halloffame: informs the score table of all users
          registered in the system.

        • l: Lists all currently "logged in" users and whether they are busy
          in a match or not.

        • call <opponent>: invites an opponent to play.He may or may not 
          accept.

            The other opponent will receive, without having to type anything,
            the following message:

            Tic-Tac-Toe> Invitation for game received from (opponent_name)!
                ...accept? (y/n):

            Then you must choose (y-yes || n-no). e.g.:

            Tic-Tac-Toe> Invitation for game received from Ripley!
                ...accept? (y/n): y (and press enter)

        • play <row> <column>: sends the play.
            In case of repeated home command provide
            Line: correct input (and press enter)
            Column: correct input (and press enter)

        • delay: during a match, informs the last 3 latency values.

        • over: ends a match early.

        • out: log off.

        • bye: ends client execution and returns to shell
          of the operating system. It is not necessary to log out in the case
          of bye without previous out.

        OBS.3: All commands above must have their respective spacing
                respected. e.g.: (new" "Ripley" "2092):
                acceptable input: Tic-Tac-Toe> new Ripley 2092

        OBS.4: No command in this program needs initial spacing.
                e.g:
 
        Tic-Tac-Toe>(Prompt space, not user typing) in Ripley 2092

  2.5 To terminate the server press "Ctrl+C" in the terminal.

  2.6 To end the client, type bye.
                acceptable input: Tic-Tac-Toe> bye

3.REFERENCES__________________________________________________________________

3.1 BIBLIOGRAPHY

    - Cooper, M. (2014). Advanced bash scripting guide: An in-depth exploration 
    of the art of shell scripting (Vol. 1). Domínio público, 10 Mar 2014.
    
    - Stevens, W., 2004. UNIX network programming. Boston: Addison-Wesley.
    
    - Kurose, J. and Ross, K., 2016. Computer Networking. Harlow, United 
    Kingdom: Pearson Education Canada.

3.2 Sites:

    Codingunit:      https://www.codingunit.com/c-tutorial-deleting-and-renaming-a-file
    Die(Dice):       https://linux.die.net/  
    Fabio Busatto:   https://tldp.org/HOWTO/TCP-Keepalive-HOWTO/programming.html
    Michael Kerrisk: https://man7.org 
    Wikipedia:       https://en.wikipedia.org 

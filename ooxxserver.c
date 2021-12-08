#if defined(_WIN32)
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0600
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")

#else
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <errno.h>

#endif


#if defined(_WIN32)
#define ISVALIDSOCKET(s) ((s) != INVALID_SOCKET)
#define CLOSESOCKET(s) closesocket(s)
#define GETSOCKETERRNO() (WSAGetLastError())

#else
#define ISVALIDSOCKET(s) ((s) >= 0)
#define CLOSESOCKET(s) close(s)
#define SOCKET int
#define GETSOCKETERRNO() (errno)
#endif


#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>
#include <signal.h>
#include <errno.h>
#include <sys/types.h>
#include<ctype.h>

struct login_info
{
    char account[100];
    char password[100];
    int login;
    char name_id[10];
    int socket;
} login[4];

void authen_login( int );
void chat( int );
void unknown( int );
void ls( int );
void challenge( int, char *);
void game_table( int );
void current_table( int, int [], char  );
void game_play( int, int , char *, char );
int isdone( int , int , int[] , int , char  );
int get_id( int  );
void logout(int);

int main() {

    strcpy(login[0].account, "apple");
    strcpy(login[0].password, "123");
    login[0].login = 0;
    strcpy(login[1].account, "banana");
    strcpy(login[1].password, "1234");
    login[1].login = 0;
    strcpy(login[2].account, "cat");
    strcpy(login[2].password, "12345");
    login[2].login = 0;

#if defined(_WIN32)
    WSADATA d;
    if (WSAStartup(MAKEWORD(2, 2), &d)) {
        fprintf(stderr, "Failed to initialize.\n");
        return 1;
    }
#endif


    printf("Configuring local address...\n");
    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    struct addrinfo *bind_address;
    getaddrinfo(0, "8080", &hints, &bind_address);


    printf("Creating socket...\n");
    SOCKET socket_listen;
    socket_listen = socket(bind_address->ai_family,
            bind_address->ai_socktype, bind_address->ai_protocol);
    if (!ISVALIDSOCKET(socket_listen)) {
        fprintf(stderr, "socket() failed. (%d)\n", GETSOCKETERRNO());
        return 1;
    }


    printf("Binding socket to local address...\n");
    if (bind(socket_listen,
                bind_address->ai_addr, bind_address->ai_addrlen)) {
        fprintf(stderr, "bind() failed. (%d)\n", GETSOCKETERRNO());
        return 1;
    }
    freeaddrinfo(bind_address);


    printf("Listening...\n");
    if (listen(socket_listen, 10) < 0) {
        fprintf(stderr, "listen() failed. (%d)\n", GETSOCKETERRNO());
        return 1;
    }

    fd_set master;
    FD_ZERO(&master);
    FD_SET(socket_listen, &master);
    SOCKET max_socket = socket_listen;

    printf("Waiting for connections...\n");


    while(1) {
        fd_set reads;
        reads = master;
        if (select(max_socket+1, &reads, 0, 0, 0) < 0) {
            fprintf(stderr, "select() failed. (%d)\n", GETSOCKETERRNO());
            return 1;
        }

        SOCKET i;
        for(i = 1; i <= max_socket; ++i) {
            if (FD_ISSET(i, &reads)) {

                if (i == socket_listen) {
                    
                    struct sockaddr_storage client_address;
                    socklen_t client_len = sizeof(client_address);
                    SOCKET socket_client = accept(socket_listen,
                            (struct sockaddr*) &client_address,
                            &client_len);

                    if (!ISVALIDSOCKET(socket_client)) {
                        fprintf(stderr, "accept() failed. (%d)\n",
                                GETSOCKETERRNO());
                        return 1;
                    }

                    FD_SET(socket_client, &master);
                    if (socket_client > max_socket)
                        max_socket = socket_client;

                    char address_buffer[100];
                    getnameinfo((struct sockaddr*)&client_address,
                            client_len,
                            address_buffer, sizeof(address_buffer), 0, 0,
                            NI_NUMERICHOST);
                    printf("New connection from %s\n", address_buffer);

                    authen_login(socket_client);

                } 
                else {
                    char read[1024];
                    memset( read, 0, sizeof(read) );
                    int bytes_received = recv(i, read, 1024, 0);
                    if (bytes_received < 1) {
                        FD_CLR(i, &master);
                        CLOSESOCKET(i);
                        continue;
                    }

                    char *lsuser = "ls";
                    char *fight = "challenge";
                    char *lgout = "logout";
                    char *chatty = "chat";
                    if( strcmp( lsuser, read ) == 0 ) 
                    {
                        ls(i);
                    }
                    else if( strcmp( fight, read ) == 0 ) 
                    {
                        challenge(i, read);
                    }
                    else if( strcmp( lgout, read ) == 0 ) 
                    {
                        
                        logout( i );
                    }
                    else if( strcmp( chatty, read) == 0) 
                    {
                        char buf[1000];
                        memset(buf, 0, sizeof(buf));
                        strcpy(buf, "Type your message......\n\0");
                        send(i, buf, strlen(buf), 0);
                        chat(i);
                    }
                    else 
                    {
                        unknown( i );
                    }

                }
            } //if FD_ISSET
        } //for i to max_socket
    } //while(1)

    printf("Closing listening socket...\n");
    CLOSESOCKET(socket_listen);

#if defined(_WIN32)
    WSACleanup();
#endif


    printf("Finished.\n");

    return 0;
}

int get_id( int client_socket )
{
    int id;
    for( int i = 0; i < 3; i++ )
    {
        if( client_socket == login[i].socket )
        {
            id = i;
            break;
        }
    }

    return id;
}

void authen_login( int client )
{
    while(1)
    {
        char msg[1000];
        memset(msg, 0, sizeof(msg));

        strcpy(msg, "Enter your account......\n\0");
        send(client, msg, strlen(msg), 0);

        char input_account[1000];
        char input_pasword[1000];
        memset(input_account, 0, sizeof(input_account));
        memset(input_pasword, 0, sizeof(input_pasword));
        int acc_r = recv( client, input_account, sizeof(input_account), 0);
    
        memset(msg, 0, sizeof(msg));
        strcpy(msg, "Enter your Password......\n\0");
        send(client, msg, sizeof(msg), 0);
        int pas_r = recv( client, input_pasword, sizeof(input_pasword), 0);
        
        int flag = 0;
        int id;

        for( int i = 0; i < 3; i++ )
        {
            if( strcmp( login[i].account, input_account) == 0 
                && strcmp( login[i].password, input_pasword ) == 0 )
            {
                flag = 1;
                id = i;
            }
        }

        if( flag == 1 )
        {
            if( login[id].login == 1 )
            {
                memset(msg, 0, sizeof(msg));
                strcpy(msg, "This account is being used! please try another one\n\0");
                send(client, msg, strlen(msg), 0);
                
                continue;
            }
            memset(msg, 0, sizeof(msg));
            strcpy(msg, "-----Login Success!-----\n-You can use 'ls' to watch who is online\n-You can use 'challenge' to challenge others\n-You can use 'chat' to chat with others\n-You can use 'logout' to Logout\n");
            send(client, msg, sizeof(msg), 0);
            login[id].login = 1;
            login[id].socket = client;
            char tmp[10];
            memset(tmp, 0, sizeof(tmp));
            sprintf(tmp, "%d", id);
            strcpy(login[id].name_id, tmp);

            
            return;
        }
        else
        {
            memset(msg, 0, sizeof(msg));
            strcpy(msg, "Wrong account or password! please try again\n\0");
            send(client, msg, strlen(msg), 0);
        }
    }
}

void chat(int client)
{
	char msg[1000];
    memset(msg, 0, sizeof(msg));
    int msg_received = recv(client, msg, 1024, 0);
    int id = get_id(client);
    char buff[1200];
    memset(buff, 0, sizeof(buff));
    sprintf(buff, "%s : %s", login[id].account, msg);

    for (int i = 0;i < 3;i++)
    {
		if (login[i].login == 1)
        {
            send(login[i].socket, buff, sizeof(msg), 0);
		}
	}
}

void unknown( int client )
{
    char msg[1000];
    memset(msg, 0, sizeof(msg));
    strcpy(msg, "This is an unknown command...\n-You can use 'ls' to watch who is online\n-You can use 'challenge' to challenge others\n-You can use 'chat' to chat with others\n-You can use 'logout' to Logout\n");
    send(client, msg, sizeof(msg), 0);
}

void ls( int client )
{

    send(client, "\nOnline User:\n=================================\n", strlen("\nOnline User:\n=================================\n"), 0);
    send(client, "id | name\n---------------------------------\n", strlen("id | name\n---------------------------------\n"), 0);

    for( int i = 0; i < 3; i++ )
    {
        if( login[i].login == 1 )
        {
            char msg[1000];
            memset(msg, 0, sizeof(msg));
            sprintf(msg, " %d : %s \n", i, login[i].account);
            send(client, msg, strlen(msg), 0);
        }
    }
}


void challenge( int client, char *read )
{
    char msg[1000];
    int client_id;
    client_id = get_id(client);

    ls(client);
    send( client, "\nChoose one's id which you want to challenge(enter 0~2)\n", strlen("\nChoose one's id which you want to challenge(enter 0~2)\n"), 0 );

    memset(read, 0, sizeof(read));
    int bytes_received = recv(client, read, 1024, 0);
    char *id = read;
    for( int i = 0; i < 3; i++ )
    {
        if( strcmp(login[i].name_id, id) == 0 )
        {
            send( client, " Waiting .......\n", strlen(" Waiting .......\n"), 0 );
            memset(msg, 0, sizeof(msg));
            sprintf( msg, "'%s' want to play a game with you (enter yes/no)\n", login[client_id].account);
            send( login[i].socket, msg, strlen(msg), 0 );

            memset(read, 0, sizeof(read));
            bytes_received = recv(login[i].socket, read, 1024, 0);
            char *ok = "yes";
            if( strcmp( ok, read ) == 0 )
            {
                send( login[i].socket, "Let's start the game(OX)\n", strlen("Let's start the game(OX)\n"), 0);
                send( login[i].socket, "---------------------------------\n", strlen("---------------------------------\n"), 0 );
                send( login[i].socket, "Waiting for your opponent choosing character.....\n", strlen("Waiting for your opponent choosing character.....\n"), 0);
                send( login[client_id].socket, "Challenge accepted!!. Let's play a game(OX)\n", strlen("Challenge accepted!!. Let's play a game(OX)\n"), 0 );
                send( login[client_id].socket, "---------------------------------\n", strlen("---------------------------------\n"), 0 );
                send( login[client_id].socket, "Pick 'o' or 'x'(enter 'o' or 'x')\n", strlen("Pick 'o' or 'x'(enter 'o' or 'x')\n"), 0);
                memset(read, 0, sizeof(read));
                bytes_received = recv(login[client_id].socket, read, 1024, 0);
                char *o = "o";
                char character;
                if( strcmp( o, read ) == 0 )
                {
                    character = 'O';
                    send( login[client_id].socket, "You are 'O'\n", strlen("You are 'O'\n"), 0);
                    send( login[i].socket, "You are 'X'\n", strlen("You are 'X'\n"), 0);
                    
                }
                else
                {
                    character = 'X';
                    send( login[client_id].socket, "You are 'X'\n", strlen("You are 'X'\n"), 0);
                    send( login[i].socket, "You are 'O'\n", strlen("You are 'O'\n"), 0);
                    
                }

                game_play( login[client_id].socket, login[i].socket, read, character );
                return;
            }
            else
            {
                send( login[client_id].socket, "Response is 'no', please try again or choose another...\n\0", strlen("Response is 'no', please try again or choose another...\n"), 0 );
                return;
            }
            
        }
    }
    
}

void game_table( int client )
{
    send( client, "\n  0  |  1  |  2  \n", strlen("\n  0  |  1  |  2  \n"), 0 );
    send( client, "-----------------\n", strlen("-----------------\n"), 0 );
    send( client, "  3  |  4  |  5  \n", strlen("  3  |  4  |  5  \n"), 0 );
    send( client, "-----------------\n", strlen("-----------------\n"), 0 );
    send( client, "  6  |  7  |  8  \n", strlen("  6  |  7  |  8  \n"), 0 );
}

void current_table( int client, int record[9], char one_pick )
{
    char another;
    if( one_pick == 'O' ) another = 'X';
    else another = 'O';
    char buf[1024];
    char tmp[9];
    memset(buf, 0, sizeof(buf));
    memset(tmp, ' ', sizeof(tmp));
    for( int i = 0; i < 9; i++ )
    {
        if( record[i] == 1 ) 
        {
            tmp[i] = one_pick;
        }
        else if( record[i] == 2 )
        {
            tmp[i] = another;
        }
    }
    sprintf(buf,"\n  %c  |  %c  |  %c  \n-----------------\n  %c  |  %c  |  %c  \n-----------------\n  %c  |  %c  |  %c  \n",
            tmp[0], tmp[1], tmp[2], tmp[3], tmp[4], tmp[5], tmp[6], tmp[7], tmp[8] );
    buf[strlen(buf)] = '\0';

    send( client, buf, strlen(buf), 0 );
}

void game_play( int inviter, int opponent, char *read, char inviter_pick )
{
    int record_table[9];
    memset(record_table, 0, sizeof(record_table));
    int bytes_received, choose, done = 0;
    while( done == 0 )
    {
        game_table(inviter);
        current_table(inviter, record_table, inviter_pick);

        send( inviter, "Your turn\n", strlen( "Your turn\n"), 0 );
        current_table(opponent, record_table, inviter_pick);
        send( opponent, "Wait for your opponent...\n", strlen( "Wait for your opponent...\n"), 0 );
        memset(read, 0, sizeof(read));
        bytes_received = recv(inviter, read, 1024, 0);
        //send(inviter, read, strlen(read), 0);
        choose = (int)read[0]%48;
        while( record_table[choose] != 0 )
        {
            send( inviter, "This place has been chosen, try another...\n", strlen( "This place has been chosen, try another...\n"), 0 );
            memset(read, 0, sizeof(read));
            bytes_received = recv(inviter, read, 1024, 0);
            choose = (int)read[0]%48;
        }
        record_table[choose] = 1;
        
        done = isdone( inviter, opponent, record_table, done, inviter_pick );
        if( done == 1 ) break;

        game_table(opponent);
        current_table(opponent, record_table, inviter_pick);

        send( opponent, "Your turn\n", strlen( "Your turn\n"), 0 );
        current_table(inviter, record_table, inviter_pick);
        send( inviter, "Wait for your opponent...\n", strlen( "Wait for your opponent...\n"), 0 );
        memset(read, 0, sizeof(read));
        bytes_received = recv(opponent, read, 1024, 0);
        choose = (int)read[0]%48;
        while( record_table[choose] != 0 )
        {
            send( opponent, "This place has been chosen, try another...\n", strlen( "This place has been chosen, try another...\n"), 0 );
            memset(read, 0, sizeof(read));
            bytes_received = recv(opponent, read, 1024, 0);
            choose = (int)read[0]%48;
        }
        record_table[choose] = 2;

        done = isdone( inviter, opponent, record_table, done, inviter_pick );
    }
    char msg[1000];
    memset(msg, 0, sizeof(msg));
    strcpy(msg, "\n-----*Game complete!*-----\n-You can use 'ls' to watch who is online\n-You can use 'challenge' to challenge others\n-You can use 'chat' to chat with others\n-You can use 'logout' to Logout\n");
    send(inviter, msg, sizeof(msg), 0);
    send(opponent, msg, sizeof(msg), 0);

}

int isdone( int client1, int client2, int record[9], int done, char pick )
{
    int flag = 0, peace = 1;
    
    if( (record[0] == 1 && record[1] == 1 && record[2] == 1) || (record[0] == 1 && record[3] == 1 && record[6] == 1)|| (record[0] == 1 && record[4] == 1 && record[8] == 1) ) 
    {
        flag = 1;
    }

    else if( record[1] == 1 && record[4] == 1 && record[7] == 1 )
    {
        flag = 1;
    } 

    else if( (record[2] == 1 && record[5] == 1 && record[8] == 1) || (record[2] == 1 && record[4] == 1 && record[6] == 1) ) 
    {
        flag = 1;
    }

    else if( record[3] == 1 && record[4] == 1 && record[5] == 1 ) 
    {
        flag = 1;
    }

    else if( record[6] == 1 && record[7] == 1 && record[8] == 1 )
    {
        flag = 1;
    }

    if( flag != 1 )
    {
        if( (record[0] == 2 && record[1] == 2 && record[2] == 2) || (record[0] == 2 && record[3] == 2 && record[6] == 2)|| (record[0] == 2 && record[4] == 2 && record[8] == 2) ) 
        {
            flag = 2;
        }

        else if( record[1] == 2 && record[4] == 2 && record[7] == 2 ) 
        {
            flag = 2;
        }

        else if( (record[2] == 2 && record[5] == 2 && record[8] == 2) || (record[2] == 2 && record[4] == 2 && record[6] == 2) )
        {
            flag = 2;
        }

        else if( record[3] == 2 && record[4] == 2 && record[5] == 2 )
        {
            flag = 2;
        }

        else if( record[6] == 2 && record[7] == 2 && record[8] == 2 )
        {
            flag = 2;
        }    
    }

    if( flag == 0 )
    {
        for( int i = 0; i < 9; i++ )
        {
            if( record[i] == 0 )
            {
                peace = 0;
                break;
            }
        }

        if( peace == 1 )
        {
            current_table(client1, record, pick);
            current_table(client2, record, pick);

            send( client1, "--------------------\n-----**Peace !!**-----\n--------------------\n", strlen("--------------------\n-----**Peace !!**-----\n--------------------\n"), 0 );
            send( client2, "--------------------\n-----**Peace !!**-----\n--------------------\n", strlen("--------------------\n-----**Peace !!**-----\n--------------------\n"), 0 );

            return 1;
        }
    }

    if( flag != 0 )
    {
        done = 1;
        current_table(client1, record, pick);
        current_table(client2, record, pick);
        if( flag == 1 )
        {
            send( client1, "------------------------------\n-----It's your Victory !!-----\n------------------------------\n", strlen("------------------------------\n-----It's your Victory !!-----\n------------------------------\n"), 0 );
            send( client2, "--------------------\n-----You Lose...-----\n--------------------\n", strlen("--------------------\n-----You Lose...-----\n--------------------\n"), 0 );
        }
        else
        {
            send( client2, "------------------------------\n-----It's your Victory !!-----\n------------------------------\n", strlen("------------------------------\n-----It's your Victory !!-----\n------------------------------\n"), 0 );
            send( client1, "--------------------\n-----You Lose...-----\n--------------------\n", strlen("--------------------\n-----You Lose...-----\n--------------------\n"), 0 );
        }
        return 1;
    }

    return 0;
}

void logout( int client )
{
    send( client, "Logout...\n", strlen("Logout...\n"), 0 );
    int client_id;
    client_id = get_id(client);
    login[client_id].login = 0;
}
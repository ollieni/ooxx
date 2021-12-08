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


#include <stdio.h>
#include <string.h>
#if defined(_WIN32)
#include <conio.h>
#endif

//设置服务器监听端口号
const unsigned long port = 8080;

int main(int argc, char *argv[]) {

#if defined(_WIN32) /*Defined as 1 when the compilation target is 32-bit ARM, 64-bit ARM, x86, or x64. Otherwise, undefined.*/
    WSADATA d;
    if (WSAStartup(MAKEWORD(2, 2), &d)) {
        fprintf(stderr, "Failed to initialize.\n");
        return 1;
    }
#endif

    //创建服务器socket，地址族为AF_INET(IPv4)，传输方式为TCP
	int server_socket;
	server_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	//初始化IP为服务器127.0.0.1，端口为已设置的port
	struct sockaddr_in server_addr;
	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(port);
	server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

	//客户端连接服务器
	connect(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr));
    printf("Connected.\n");
    printf("To send data, enter text followed by enter.\n");

    while(1) {

        fd_set reads;
        FD_ZERO(&reads);
        FD_SET(server_socket, &reads);
#if !defined(_WIN32)
        FD_SET(0, &reads);
#endif

        struct timeval timeout;
        timeout.tv_sec = 10;
        timeout.tv_usec = 100000;

        if (select(server_socket+1, &reads, 0, 0, &timeout) < 0) {
            fprintf(stderr, "select() failed. (%d)\n", GETSOCKETERRNO());
            return 1;
        }

        if (FD_ISSET(server_socket, &reads)) {
            char read[4096];
            char *logout = "Logout...\n";
            memset(read,'\0',sizeof(read));
            int bytes_received = recv(server_socket, read, 4096, 0);
            if (bytes_received < 1) {
                printf("Connection closed by peer.\n");
                break;
            }
            printf("\n%s\n", read);
            if( strcmp( logout, read ) == 0 ) break;    /*收到logout就結束*/
        }

#if defined(_WIN32)
        if(_kbhit()) {
#else
        if(FD_ISSET(0, &reads)) {
#endif
            char read[4096];
            if (!fgets(read, 4096, stdin)) break;

            if( read[strlen(read)-1] == '\n' ) read[strlen(read)-1] = '\0';     /*把收到的命令尾端設為\0*/
            int bytes_sent = send(server_socket, read, strlen(read), 0);        /*把命令送去server*/

        }
    } //end while(1)

    printf("Closing socket...\n");
    CLOSESOCKET(server_socket);

#if defined(_WIN32)
    WSACleanup();
#endif

    printf("Finished.\n");
    return 0;
}
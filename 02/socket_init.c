/*sock_init.c*/

#if defined(_WIN32)
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0600 //This macro defines the minimmum windows version this code supports
#endif
#include <WinSock2.h>
#include <WS2tcpip.h>
#pragma comment(lib, "ws2_32.lib")

#else   //If the program is not compiled on Windows, then this section will compile
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <errno.h>
#endif

#include <stdio.h>

int main()
{

#if defined(_WIN32)
    WSADATA d;
    int errCode = WSAStartup(MAKEWORD(2, 2), &d); 
    if(errCode)
    {
        printf("Failed to initialize.\n");
        return 1;
    }
#endif
    printf("Return value: %d", errCode);
    printf(" Ready to use Socket API.\n");

#if defined(_WIN32)
    WSACleanup();
#endif

    return 0;
}
/* time_server.c */

//For Windows
#if defined(_WIN32)
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0600
#endif
#include <WinSock2.h>
#include <WS2tcpip.h>
#pragma comment(lib, "ws2_32.lib")

//For Linux based OS
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
#include <time.h>

int main()
{
#if defined(_WIN32)
    WSADATA d;
    if(WSAStartup(MAKEWORD(2, 2), &d))
    {
        //fprintf(stderr, "Failed to initialize.\n");
        printf("Failed to initialize.\n");
        return 1;
    }
#endif

    printf("Configuring local server...\n");
    struct addrinfo hints;
    //Initialize the 'hints' struct with 0s
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;   // This indicates that we're going to be using TCP
    hints.ai_flags = AI_PASSIVE;    //This so we can listen on any available network interface

    struct addrinfo *bind_address;
    getaddrinfo(0, "8080", &hints, &bind_address);
    
    printf("Creating socket...\n");
    SOCKET socket_listen;
    socket_listen = socket(bind_address->ai_family, bind_address->ai_socktype, bind_address->ai_protocol);
    if(!ISVALIDSOCKET(socket_listen))
    {
        printf("socket() failed. (%d)\n", GETSOCKETERRNO());
        return 1;
    }    

    printf("Binding socket to a local address...\n");
    if(bind(socket_listen, bind_address->ai_addr, bind_address->ai_addrlen))
    {
        //output with stderr shows cosmetic error therefore avoiding its usage
        //fprintf(stderr ,"bind() failed. (%d)\n", GETSOCKETERRNO());
        printf("bind() failed. (%d)\n", GETSOCKETERRNO());
        return 1;
    }
    freeaddrinfo(bind_address);

    printf("Listening on http://127.0.0.1:8080 ...\n");
    if(listen(socket_listen, 10) < 0)
    {
        printf("listen() failed. (%d)\n", GETSOCKETERRNO());
        return 1;
    }
    
    printf("Waiting for connection...\n");
    struct sockaddr_storage client_address;
    socklen_t client_len = sizeof(client_address);
    SOCKET socket_client = accept(socket_listen, (struct sockaddr*)&client_address, &client_len);
    if(!ISVALIDSOCKET(socket_client))
    {
        printf("accept() failed. (%d)\n", GETSOCKETERRNO());
        return 1;
    }

    printf("Client is connected...");
    char address_buffer[100];   //Just to log client/host connection address info
    getnameinfo((struct sockaddr*)&client_address,
                    client_len, address_buffer, sizeof(address_buffer), 0, 0, NI_NUMERICHOST);
    printf("%s\n", address_buffer);

    //Recieving Http requests from the client
    printf("Reading request...\n");
    char request[1024];
    int bytes_recieved = recv(socket_client, request, 1024, 0);
    //We dont care about the request type at this point so we'll just ignore it
    //printf("%.*s", bytes_recieved, request);
    printf("Recieved %d bytes.\n", bytes_recieved);

    //We just send our response
    printf("Sending response...\n");
    //Building a http response 
    const char *response =
        "HTTP/`.` 200 OK\r\n"
        "Connection: close\r\n"
        "Content-Type: text/plain\r\n\r\n"
        "Local time is: ";

    int bytes_sent = send(socket_client, response, strlen(response), 0);
    printf("Send %d of %d bytes.\n", bytes_sent, (int)strlen(response));

    time_t timer;
    time(&timer);
    char* time_msg = ctime(&timer);
    bytes_sent = send(socket_client, time_msg, strlen(time_msg), 0);
    printf("Sent %d of %d bytes.\n", bytes_sent, (int)strlen(time_msg));

    //Terminating connection with the client 
    printf("Closing connection...\n");
    CLOSESOCKET(socket_client);

    //Turning off our server 
    printf("Closing the listening socket...\n");
    CLOSESOCKET(socket_listen);

#if defined(_WIN32)
    WSACleanup();
#endif

    printf("Finished. :) \n");

    return 1;
}




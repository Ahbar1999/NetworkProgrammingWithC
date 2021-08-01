/*ToUpper tcp server*/

#include "../portable_headers.h"
#include <ctype.h>

int main()
{
#if defined(_WIN32)
    WSADATA d;
    if(WSAStartup(MAKEWORD(2, 2), &d))
    {
        fprintf(stderr, "Startup failed!\n");
        return 1;
    }
#endif

    printf("Configuring local address...\n");
    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;  //IPv4
    hints.ai_socktype = SOCK_STREAM;    //TCP proto
    hints.ai_flags = AI_PASSIVE;

    struct addrinfo *bind_address;
    getaddrinfo(0, "8080", &hints, &bind_address);

    printf("Creatig socket...\n");
    SOCKET socket_listen;
    socket_listen = socket(bind_address->ai_family, bind_address->ai_socktype, bind_address->ai_protocol);
    if(!ISVALIDSOCKET(socket_listen))
    {
        fprintf(stderr, "socket() failed. (%d)\n", GETSOCKETERRNO());
        return 1;
    }

    printf("Binding socket to a local address...\n");;
    if(bind(socket_listen, bind_address->ai_addr, bind_address->ai_addrlen))
    {
        fprintf(stderr, "bind() failed (%d).\n", GETSOCKETERRNO());
        return 1;
    }

    freeaddrinfo(bind_address);
    //Finally put the socket into listening mode
    printf("Listening...\n");
    if(listen(socket_listen, 10) < 0)
    {
        fprintf(stderr, "listen() failed (%d)", GETSOCKETERRNO());
        return 1;
    }
    //Now our only socket is in listening state

    //Create an fd set to run select on
    fd_set master;
    FD_ZERO(&master);
    FD_SET(socket_listen, &master);
    SOCKET max_socket = socket_listen;  //This to store max socket descriptor for further usage

    printf("Waiting for connections..\n");
    while(TRUE)
    {
        
        fd_set reads;   //set of sockets to read from
        reads = master;
        fd_set writes;  //set of sockets to write to  
        writes = master;
        if(select(max_socket+1, &reads, &writes, 0, 0) < 0)
        {
            fprintf(stderr, "select() failed. (%d)\n", GETSOCKETERRNO());
            return 1;
        }

        //Loop through all the sockets to check which ones are ready
        SOCKET i;
        for(i= 1; i<=max_socket; ++i)
        {
            if(FD_ISSET(i, &reads))
            {
                //Perform operation based on the type of socket 'i' is
                if(i == socket_listen)  //If some client is trying to connect
                {
                    struct sockaddr_storage client_address;
                    socklen_t client_len = sizeof(client_address);
                    SOCKET socket_client = accept(socket_listen,
                                                    (struct sockaddr*)&client_address,
                                                    &client_len);
                    if(!ISVALIDSOCKET(socket_client))
                    {
                        fprintf(stderr, "accept() failed (%d).", GETSOCKETERRNO());
                        return 1;
                    }
                    
                    //Add our newly created client socket to the master set
                    FD_SET(socket_client, &master);
                    if(socket_client > max_socket) max_socket = socket_client;

                    //Client conection info for debug pruposes
                    char address_buffer[100];
                    getnameinfo((struct sockaddr*)&client_address, client_len, address_buffer, sizeof(address_buffer), 0, 0, NI_NUMERICHOST);
                    printf("New connection from %s\n", address_buffer);
                }
                else    //If the socket is not socket_listen / if the ready socket is client socket and is ready to recv data
                {
                    //Perform the to upper operation
                    //Array to hold recieved data                
                    char read[1024];
                    int bytes_received = recv(i, read, 1024, 0);

                    //If the recieved data is indicative of closed connection
                    if(bytes_received < 1)
                    {
                        FD_CLR(i, &master);
                        CLOSESOCKET(i);
                        continue;
                    }     

                    //If connection hasnt been closed
                    //Relay the recvd data from one client socket to all the other 
                    for(SOCKET j =1; j<=max_socket; ++j)
                    {
                        if(FD_ISSET(j, &writes))
                        {
                            if(j != i && j != socket_listen)    //to make sure we dont send data to server and sender socket
                            {
                                send(j, read, bytes_received, 0);
                            }
                        }
                    }
                }
            }//If FD_ISSET()
        }//for i to max_socket
    }//while(TRUE), this while loop will never end and will always listen for new connections

    printf("Closing listening socket.\n");
    CLOSESOCKET(socket_listen);

#if defined(_WIN32)
    WSACleanup();
#endif

    printf("Finished.\n");
    return 0;
}
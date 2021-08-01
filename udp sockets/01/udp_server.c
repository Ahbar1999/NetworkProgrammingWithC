/*A simple udp server*/

#include "../../portable_headers.h"

int main()
{
#if defined(_WIN32)
    WSADATA d;
    if(WSAStartup(MAKEWORD(2, 2) ,&d))
    {
        fprintf(stderr, "Startup failed.\n");
        return 1;
    }
#endif

    //Configuring local address
    printf("Configuring local address...\n");
    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints)); 
    hints.ai_family = AF_INET;
    hints.ai_socktype= SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE;
    struct addrinfo *bind_address;
    getaddrinfo(0, "8080", &hints, &bind_address);
    
    printf("Creating socket...\n");
    SOCKET socket_listen;
    socket_listen = socket(bind_address->ai_family, bind_address->ai_socktype, bind_address->ai_protocol);
    if(!ISVALIDSOCKET(socket_listen))
    {
        fprintf(stderr, "socket() failed (%d)\n", GETSOCKETERRNO());
        return 1;
    }

    printf("Binding socket to a local address...\n");
    if(bind(socket_listen, bind_address->ai_addr, bind_address->ai_addrlen))
    {
        fprintf(stderr, "bind() failed (%d). \n", GETSOCKETERRNO());
        return 1;
    }
    // char addr_node[1024];
    // char addr_serv[1024];
    // getnameinfo(bind_address->ai_addr, bind_address->ai_addrlen, addr_node, sizeof(addr_node), addr_serv, sizeof(addr_serv), NI_NUMERICHOST | NI_NUMERICSERV);
    // printf("Listening on: %s/ %s", addr_node, addr_serv);
    freeaddrinfo(bind_address);

    struct sockaddr_storage client_address;
    socklen_t client_len = sizeof(client_address);
    char read[1024];
    //recvfrom returns sender's address and received data unlike the recv() in tcp programs
    int bytes_received = recvfrom(socket_listen, read, 1024, 0, (struct sockaddr*)&client_address, &client_len);

    //print the received data
    printf("Received (%d bytes): %.*s\n", bytes_received, bytes_received, read);

    //Print the senders address
    printf("Remote address is: ");
    char address_buffer[100];
    char service_buffer[100];
    getnameinfo((struct sockaddr*)&client_address, client_len,  address_buffer, sizeof(address_buffer), service_buffer, sizeof(service_buffer), NI_NUMERICHOST | NI_NUMERICSERV);
    printf("%s %s\n", address_buffer, service_buffer);

    //Cleanup
    CLOSESOCKET(socket_listen);
#if defined(_WIN32)
    WSACleanup();
#endif
    
    printf("Finished.\n");

    return 0;
}
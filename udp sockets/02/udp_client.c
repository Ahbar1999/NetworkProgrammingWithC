#include "../../portable_headers.h"


int main()
{
#if defined(_WIN32)
    WSADATA d;
    if(WSAStartup(MAKEWORD(2, 2), &d))
    {
        fprintf(stderr, "Strartup failed. \n");
        return 1;
    }
#endif

    printf("Configuring remote address...\n");
    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_socktype = SOCK_DGRAM;
    struct addrinfo *peer_address;
    if(getaddrinfo("127.0.0.1", "8080", &hints, &peer_address))
    {
        fprintf(stderr, "getaddrinfo failed (%d).\n", GETSOCKETERRNO());
        return 1;
    }
    
    printf("Creating socket...\n");
    SOCKET socket_peer;
    socket_peer = socket(peer_address->ai_family, peer_address->ai_socktype ,peer_address->ai_protocol);
    if(!ISVALIDSOCKET(socket_peer))
    {
        fprintf(stderr, "socket() failed (%d)\n", GETSOCKETERRNO());
        return 1;
    }

    //Once the socket has been created we can start sending data 

    const char* message = "Hello from the server side.";
    printf("Sending: %s\n", message);
    int bytes_sent = sendto(socket_peer, message, strlen(message), 0, peer_address->ai_addr, peer_address->ai_addrlen);

    printf("Sent %d bytes of data.", bytes_sent);

    char read[4096];
    int bytes_received = recv(socket_peer, read, 4096, 0);

    printf("Received (%d bytes): %.*s\n",
            bytes_received, bytes_received, read);
    
    CLOSESOCKET(socket_peer);

#if defined(_WIN32)
    WSACleanup();
#endif

    printf("Finished.");
    return 0;
}
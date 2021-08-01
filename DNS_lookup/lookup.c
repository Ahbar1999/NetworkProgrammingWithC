/*lookup.c*/

//A basic dns loopkup program that gets the ip address of a hostname/host

#include "../portable_headers.h"

#ifndef AI_ALL
#define AI_ALL 0x0100       //Some systems might not have this defined 
#endif

int main(int argc, char* argv[])
{   
    //If required number of cmd line arguments are not recvd
    if(argc < 2)
    {
        printf("Usage:\n\tlookup hostname\n");
        printf("Example:\n\tlookup example.com\n");
        exit(0);
    }

#if defined(_WIN32)
    WSADATA d;
    if(WSAStartup(MAKEWORD(2, 2), &d))
    {
        fprintf(stderr, "Startup failed.\n");
        return 1;
    }
#endif

    printf("Resolving hostname '%s'\n", argv[1]);
    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_flags = AI_ALL;     //GIVE US ALL KINDS OF ADDRESS(IPV4 AND IPV6)
    struct addrinfo* peer_address;
    if(getaddrinfo(argv[1], 0, &hints, &peer_address))
    {
        fprintf(stderr, "getaddrinfo() failed (%d) \n", GETSOCKETERRNO());
        return 1;
    }

    printf("Remote address is:\n");
    struct addrinfo* address = peer_address;
    do
    {
        char address_buffer[100];
        
        getnameinfo(address->ai_addr, address->ai_addrlen, address_buffer, sizeof(address_buffer), 0, 0, NI_NUMERICHOST);
        printf("%s\n", address_buffer);

    } while (address = address->ai_next);   //while next address is not null keep printing adresses
    freeaddrinfo(peer_address);

#if defined(_WIN32)
    WSACleanup();
#endif

    printf("Finished\n");

    return 0;
}
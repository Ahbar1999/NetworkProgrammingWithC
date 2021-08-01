
/*
    WINSOCK is Windows Networking API 
*/

#include <stdio.h>
#include <winsock2.h>
#pragma comment(lib, "ws2_32.lib")

int main()
{
    //A data structure that holds information about the Windows socket implementation
    WSADATA d;

    //WSAStartup initializes the WINSOCK api 
    //With the requested version and the a data container passed as args
    if(WSAStartup(MAKEWORD(2, 2), &d))
    {
        //If failed to initialized then return -1 and exit 
        printf("Failed to initialize.\n");
        return -1;
    }

    //WSACleanup called when a winsock program is finished 
    WSACleanup();
    printf("Ok.\n");

    return 0;
}
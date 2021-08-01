#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0600
#endif

#include <winsock2.h>
#include <iphlpapi.h>
#include <ws2tcpip.h>
#include <stdio.h>
#include <stdlib.h>

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "iphlpapi.lib")

int main() {

    WSADATA d;
    if (WSAStartup(MAKEWORD(2, 2), &d)) 
    {
        printf("Failed to initialize.\n");
        return -1;
    }

    DWORD asize = 20000;
    //This is basically a ptr to the linked list of adapters' addresses
    PIP_ADAPTER_ADDRESSES adapters;
    do 
    {
        //Allocate memory for address
        adapters = (PIP_ADAPTER_ADDRESSES)malloc(asize);

        if (!adapters) {
            printf("Couldn't allocate %ld bytes for adapters.\n", asize);
            WSACleanup();
            return -1;
        }

        //Get the address in the adapters variable
        int r = GetAdaptersAddresses(AF_UNSPEC, GAA_FLAG_INCLUDE_PREFIX, 0, adapters, &asize);
        if (r == ERROR_BUFFER_OVERFLOW) 
        {
            //If 'ERROR_BUFFER_OVERFLOW' is encountered
            //'asize' is updated with the required size
            printf("GetAdaptersAddresses wants %ld bytes.\n", asize);
            free(adapters);
        } 
        else if (r == ERROR_SUCCESS) 
        {
            break;
        } 
        else 
        {
            printf("Error from GetAdaptersAddresses: %d\n", r);
            free(adapters);
            WSACleanup();
            return -1;
        }
    } while (!adapters);

    //Ptr to the first node
    PIP_ADAPTER_ADDRESSES adapter = adapters;
    //Loop through all the adapters
    while (adapter) 
    {
        printf("\nAdapter name: %S\n", adapter->FriendlyName);
        PIP_ADAPTER_UNICAST_ADDRESS address = adapter->FirstUnicastAddress;
        
        //Loop through all the addresses of an adapter
        while (address) 
        {
            //Check for ipv4/ipv6 label
            printf("\t%s", address->Address.lpSockaddr->sa_family == AF_INET ? "IPv4" : "IPv6");

            //This is for storing address info
            char ap[100];

            //Get the address
            getnameinfo(address->Address.lpSockaddr, address->Address.iSockaddrLength, ap, sizeof(ap), 0, 0, NI_NUMERICHOST);
            
            //Print the address
            printf("\t%s\n", ap);

            //Update the address 
            address = address->Next;
        }
        //Update the adapter
        adapter = adapter->Next;
    }

    //Finish Up
    free(adapters);
    WSACleanup();
    return 0;
}

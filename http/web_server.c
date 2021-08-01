#include "http_server_header.h"

int main()
{
#if defined(_WIN32)
    WSADATA d;
    if(WSAStartup(MAKEWORD(2, 2), &d))
    {
        fprintf(stderr, "startup failed.\n");
        return 1;
    }
#endif
                                //127.0.0.1 for serving on localhost only
    SOCKET server = create_socket(0, "8080");

    while(1)
    {
        //ENDLESS LOOP
        fd_set reads;
        reads = wait_on_clients(server);

        if(FD_ISSET(server, &reads))
        {   
            //IF A NEW CONNECTION IS READY TO BE ESTABLISHED
            struct client_info *client =   get_client(-1);  //-1 so the get_client creates a new client 
            client->socket =  accept(server, (struct sockaddr*)&(client->address), &(client->address_length)); 

            if(!ISVALIDSOCKET(client->socket))
            {
                fprintf(stderr, "accept() failed. (%d)\n", GETSOCKETERRNO());
                return 1;
            }

            printf("New connection from %s.\n", get_client_address(client));
        }

        struct client_info *client = clients;
        while(client)
        {
            struct client_info *next = client->next;
            //IF NEW DATA IS READY TO BE READ FROM EXISTING CONNECTIONS
            if(FD_ISSET(client->socket, &reads))
            {
                if(MAX_REQUEST_SIZE == client->received)    //If we have recieved more than max data from this client then send_400()
                {
                    send_400(client);
                    client = next;
                    continue;
                }

                //Otherwise we recv more data from this client
                int r  = recv(client->socket, client->request + client->received, MAX_REQUEST_SIZE-client->received, 0);

                if(r < 1)
                {
                    //If read bytes is less than 1
                    printf("Unexpected disconnected from %s.\n", get_client_address(client));
                    drop_client(client);
                }
                else
                {
                    client->received += r;
                    client->request[client->received]  =0;  //to create null terminated string
                    
                    char *q = strstr(client->request, "\r\n\r\n");
                    if(q)
                    {
                        *q = 0;
                        
                        //If we find a delineated 'rnrn' we can begin to parse the header
                        if(strncmp("GET /", client->request, 5))
                        {
                            send_400(client);
                        }
                        else
                        {
                            char *path = client->request + 4;
                            char *end_path = strstr(path, " ");
                            if(!end_path)
                            {
                                send_400(client);
                            }
                            else
                            {
                                *end_path = 0;
                                server_resource(client, path);
                            }
                        }
                    }//if(q)
                }
            }   
            client = next;
        }
    }//while(1)

    printf("\nClosing socket...\n");
    CLOSESOCKET(server);

#if defined(_WIN32)
    WSACleanup();
#endif

    printf("Finished.\n");
    return 0;
}
#include "../portable_headers.h"

#define TIMEOUT 5.0

void parse_url(char* url, char** hostname, char** port, char** path)
{
    printf("URL: %s\n", url);

    char* p;
    p = strstr(url, "://");

    char* protocol = 0;
    if(p)   //if p is not null/ if :// is found
    {
        protocol = url;
        *p = 0; //set ":" in the url string to 0
        p += 3; //p points to the character after '0//' i.e the starting of the hostname
    }
    else
    {
        //If not found, set p back to the beginning of the url
        p = url;
    }
    if(protocol)
    {   
        //Check if the found protocol is http or not
        if(strcmp(protocol, "http"))
        {
            fprintf(stderr, "Unknown protocol '%s', only http is supported.\n", protocol);
            exit(1);
        }
    }
    printf("Parsed protocol...\n");

    *hostname = p;
    //printf("Parsed hostname...\n");
    while (*p && *p != ':' && *p != '/' && *p != '#') ++p;  //look for a port
    *port = "80";
    if(*p == ':')
    {
        *p++ = 0;  //set ':' = 0 and move the pointer to port number's first character
        *port = p;  //set the port ptr to p
    }
    while(*p && *p != '/' && *p != '#') ++p;   //look for path  
    printf("Parsed port number...\n");

    *path = p;
    if(*p == '/')
    {
        *path = p+1;
    }
    *p  =0; //replace the interrupt character with 0
    printf("Parsed path...\n");

    while(*p && *p != '#') ++p;
    if(*p == '#') *p = 0;   //replace the null or "#" to 0

    printf("Parsing finshed!\n");
    //now the original url string has been modified where characters like '/', '#' have been set to 0/null
    printf("hostname: %s\n", *hostname);
    printf("port: %s\n", *port);
    printf("path: %s\n", *path);
}

//Builds an http request and sends it through the peer_socket
void send_request(SOCKET s, char *hostname, char *port, char *path)
{
    char buffer[2048];

    sprintf(buffer, "GET /%s HTTP/1.1\r\n", path);
    sprintf(buffer+ strlen(buffer), "Host: %s:%s\r\n", hostname, port);
    sprintf(buffer+ strlen(buffer), "Connection: close\r\n");
    sprintf(buffer+ strlen(buffer),"User-Agent: honpwc web_get 1.0\r\n");
    sprintf(buffer+ strlen(buffer), "\r\n");

    send(s, buffer, strlen(buffer), 0);
    printf("Sent Headers: \n%s", buffer);
}

//This helper function tries to establish a tcp connection to the host and returns a socket 
SOCKET connect_to_host(char *hostname, char* port)
{
    printf("Configuring remote address...\n");
    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_socktype = SOCK_STREAM;
    struct addrinfo* peer_address;
    if(getaddrinfo(hostname, port, &hints, &peer_address))
    {
        fprintf(stderr, "getaddrinfo() failed. (%d)\n", GETSOCKETERRNO());
        return 1;
    }
    
    //Print the remote address 
    printf("Remote address is: ");
    char address_buffer[100];
    char service_buffer[100];
    getnameinfo(peer_address->ai_addr, peer_address->ai_addrlen, address_buffer, sizeof(address_buffer), service_buffer, sizeof(service_buffer), NI_NUMERICHOST);
    printf("%s %s\n", address_buffer, service_buffer);

    printf("Creating socket...\n");
    SOCKET server;
    server = socket(peer_address->ai_family, peer_address->ai_socktype, peer_address->ai_protocol);
    if(!ISVALIDSOCKET(server))
    {
        fprintf(stderr, "socket() failed (%d)", GETSOCKETERRNO());\
        return 1;
    }

    printf("Connecting...\n");
    if(connect(server, peer_address->ai_addr, peer_address->ai_addrlen))
    {
        fprintf(stderr, "connect() failed (%d)", GETSOCKETERRNO());
        return 1;
    }
    freeaddrinfo(peer_address);

    printf("Connected.\n");

    return server;
}


int main(int argc, char* argv[]) 
{          
#if defined(_WIN32)
    WSADATA d;
    if(WSAStartup(MAKEWORD(2, 2), &d))
    {
        fprintf(stderr, "startup failed.\n");
        return 1;
    }
#endif

    //If invalid args passed 
    if(argc < 2)
    {
        fprintf(stderr, "usage: web_get url\n");
        return 1;
    }
    char* url = argv[1];

    char *hostname, *port, *path;
    parse_url(url, &hostname, &port, &path);

    SOCKET server = connect_to_host(hostname, port);
    send_request(server, hostname, port, path);

    //start time
    const clock_t start_time = clock();

#define RESPONSE_SIZE 8192  //Http response max size
    
    char response[RESPONSE_SIZE+1];
    char *p = response, *q; //P will point to the position currently being parsed
    char *end = response + RESPONSE_SIZE;   //Points to the end of the response
    char *body  = 0;    //Will point to the start of body 

    enum {length, chunked, connection};
    int encoding = 0;   //Will refer to one of the enums defined above 
    int remaining = 0;  //Will store remaining bytes needed to finish http body

    //Read loop starts
    while(1)
    {
        //First we check for timeout conditions
        if((clock() - start_time)/CLOCKS_PER_SEC > TIMEOUT)
        {
            fprintf(stderr, "timed out after %.2f seconds\n", TIMEOUT);
            return 1;
        }
        //Then we check if we havent ran out of alloted buffer space
        if(p == end)
        {
            fprintf(stderr, "out of buffer space. \n");
            return 1;
        }

        //Multiplexing
        fd_set reads;
        FD_ZERO(&reads);
        FD_SET(server, &reads);

        struct timeval timeout;
        timeout.tv_sec = 0;
        timeout.tv_usec = 200000;

        if(select(server+1, &reads, 0, 0, &timeout) < 0)
        {
            fprintf(stderr, "select() failed (%d)\n", GETSOCKETERRNO());
            return 1;
        }

        if(FD_ISSET(server, &reads))
        {
            int bytes_received = recv(server, p, end-p, 0);
            if(bytes_received < 1)
            {
                if(encoding == connection && body)  //If encoding == connection and a body has been found
                {
                    printf("%.*s", (int)(end-body), body);  //Print the remaining data which wasnt considered as part of the body in parsing 
                }

                printf("\nConnection closed by the peer.\n");
                break;
            }
            // printf("Received (%d bytes) : '%.*s", bytes_received, bytes_received, p);
        
            p += bytes_received;
            *p = 0; //To end our data with null character

            if(!body && (body = strstr(response, "\r\n\r\n")))  //double line end marks the end of header
            {
                *body = 0;  //update the end of header to null character
                body += 4;  //set the body pointer to the start of the body, skipping the 4 escape characters
                //Print the header for debugging purposes
                printf("Received Headers: \n%s\n", response);  

                q = strstr(response, "\nContent-Length: ");
                if(q)
                {
                    encoding = length;
                    q = strchr(q, ' '); //Example: Content-Length: 1256, q will point to the space before '1'
                    q += 1; //Now q points to '1', it basically points to integer array
                    remaining = strtol(q, 0 , 10);  //Since the size is already there therefore we need to treat is as decimal               
                }
                else
                {
                    q = strstr(response, "\nTranser-Encoding: chunked");
                    if(q)
                    {
                        encoding  = chunked;
                        remaining  = 0; //chunk length hasnt been set yet
                    }
                    else    //No body is found
                    {
                        encoding = connection;  //This indicates: when the connection has been closed the body has been read  
                    }
                }
                printf("\nRecieved Body:\n");
            }

            if(body)    //If the body has been found
            {
                if(encoding == length)
                {
                    if(remaining <= p -body)
                    {
                        printf("%.*s", remaining, body);
                        break;
                    }
                }
                else if(encoding == chunked)
                {
                    do
                    {
                        if(remaining == 0)  //Initialy if chunk length is zero then program is waiting to be read
                        {
                            if((q = strstr(body, "\r\n")))
                            {
                                remaining = strtol(body, 0, 16);    //hexadecimal chunk
                                if(!remaining) goto finish; //if chunk length == 0, chunk is finished
                                body = q + 2;  
                            }
                            else
                            {
                                break;
                            }
                        }
                        if (remaining && p- body >= remaining)
                        {
                            printf("%.*s", remaining, body);
                            body += remaining + 2;
                            remaining = 0;
                        }
                    }while(!remaining); //Chunk is terminated by zero length chunk   
                }
            }// if(body)
        }//if(FD_ISSET)
    }//end while(1)

finish:

    printf("\nClosing socket...\n");
    CLOSESOCKET(server);

#if defined(_WIN32)
    WSACleanup();
#endif

    printf("Finished.\n");
    return 0;
}

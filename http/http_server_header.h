#include "../portable_headers.h"

#define MAX_REQUEST_SIZE 2047   //max request size for any http request 

//A LINKED LIST DATA STRUCTURE FOR STORING CLIENTS INFO
//This struct stores the info about a client connected to our server
struct client_info
{
    socklen_t address_length;   //Length of client's socket address
    struct sockaddr_storage address;    //Client's address
    SOCKET socket;  //Actual socket 
    char request[MAX_REQUEST_SIZE]; //The data received from the client socket so far, basically acts as a buffer
    int received;   //Number of bytes stored in the array 
    struct client_info *next;   //Ptr to next node(client)
};

static struct client_info *clients = 0; //GLOBAL PTR TO ROOT NODE FOR CLIENTS' LINKED LIST  

//THE FOLLOWING ARE THE HELPER FUNCTIONS THAT WE WILL BE USING FOR/ALONGSIDE LINKED LIST OF OUR CLIENTS
/*

get_client() -> returns a client_info about a client
drop_client() -> removes a client from the list and closes the connection
get_client_address() -> returns the IP address of a client in the list
wait_on_clients() -> uses select() on the client to wait for response from a client
send_400() -> self explanatory
serve_resource() -> attempts to transfer a file to the client

*/

//IMPLEMENTATIONS OF THE ABOVE HELPER FUNCTIONS

const char *get_content_type(const char* path)
{
    //This hold the the position of the . in the filname
    const char* last_dot  = strchr(path, '.');
    if(last_dot)
    {
        if (strcmp(last_dot, ".css") == 0) return "text/css";
        if (strcmp(last_dot, ".csv") == 0) return "text/csv";
        if (strcmp(last_dot, ".gif") == 0) return "image/gif";
        if (strcmp(last_dot, ".htm") == 0) return "text/html";
        if (strcmp(last_dot, ".html") == 0) return "text/html";
        if (strcmp(last_dot, ".ico") == 0) return "image/x-icon";
        if (strcmp(last_dot, ".jpeg") == 0) return "image/jpeg";
        if (strcmp(last_dot, ".jpg") == 0) return "image/jpeg";
        if (strcmp(last_dot, ".js") == 0) return "application/javascript";
        if (strcmp(last_dot, ".json") == 0) return "application/json";
        if (strcmp(last_dot, ".png") == 0) return "image/png";
        if (strcmp(last_dot, ".pdf") == 0) return "application/pdf";
        if (strcmp(last_dot, ".svg") == 0) return "image/svg+xml";
        if (strcmp(last_dot, ".txt") == 0) return "text/plain";
    }
        return "application/octet-stream";
}

//get_client()
struct client_info *get_client(SOCKET s)
{
    struct client_info *ci = clients;

    while(ci)
    {
        if(ci->socket == s) break;
        ci = ci->next;
    }

    //If found, return it
    if(ci) return ci;
    
    //If not found, create a new one and return it 
    //calloc() fn basically is malloc but it also intializes the values to 0
    struct client_info *n = (struct client_info*) calloc(1, sizeof(struct client_info));
    if(!n)  //If memory allocation failed
    {
        fprintf(stderr, "Out of memory.\n");
        exit(1);
    }

    n->address_length = sizeof(n->address);
    n->next = clients;  //Insert the new node at head of the list 
    clients = n; //set the clien ptr back to head

    return n;
}

//drop_client()
void drop_client(struct client_info *client)
{
    CLOSESOCKET(client->socket);

    struct client_info **p = &client;

    while(*p)
    {
        if(*p == client)
        {
            *p = client->next;
            free(client);
            return;
        }
        p = &(*p)->next;
    }

    fprintf(stderr, "drop_client not found.\n");
    exit(1);
}

//get_client_adress()
const char *get_client_address(struct client_info *ci)
{
    //This char array is declared static, which ensures that its memory is available after the function returns
    static char address_buffer[100];
    getnameinfo((struct sockaddr*)&ci->address, ci->address_length, address_buffer, sizeof(address_buffer), 0, 0, NI_NUMERICHOST);

    return address_buffer;
}

//wait on clients
fd_set wait_on_clients(SOCKET server)
{
    fd_set reads;
    FD_ZERO(&reads);
    FD_SET(server, &reads);
    SOCKET max_socket = server;

    struct client_info *ci = clients;
    
    //This loops basically finds the max_socket for further uses   
    while(ci)
    {
        FD_SET(ci->socket, &reads);
        if(ci->socket > max_socket) max_socket = ci->socket;
        ci = ci->next;
    }

    if(select(max_socket+1, &reads, 0, 0, 0) < 0)
    {
        fprintf(stderr, "select() failed. (%d).", GETSOCKETERRNO());
        exit(1);
    }

    return reads;
}

//send_400()
void send_400(struct client_info *client)
{   
    const char *c400 = "HTTP/1.1 400 Bad Request\r\n"
        "Connection: close\r\n"
        "Content-Length: 11\r\n\r\nBad Request";

    send(client->socket, c400, strlen(c400), 0);

    drop_client(client);
}

//send_404()
void send_404(struct client_info *client)
{
    const char *c404 = "HTTP/1.1 404 Not Found\r\n"
        "Connection: close\r\n"
        "Content-Length: 9\r\n\r\nNot Found";
    
    send(client->socket, c404, strlen(c404), 0);
    drop_client(client);
}

//serve_resource()
void server_resource(struct client_info *client, const char *path)
{
    printf("serve_resource %s %s\n", get_client_address(client), path);
    
    //If default file requested 
    if(strcmp(path, "/") == 0) path = "/index.html";

    //If path too long
    if(strlen(path) > 100)
    {
        send_400(client);
        return;
    }
    //If path request from parents directory
    if(strstr(path, ".."))
    {
        send_404(client);
        return;
    }

    char full_path[128];
    sprintf(full_path, "public%s", path);

#if defined(_WIN32)
    char *p = full_path;
    while(*p)
    {
        if(*p == '/') *p = '\\';    // '//'- > this is basically one backslash 
        ++p;
    }    
#endif

    FILE *fp = fopen(full_path, "rb");
    if(!fp)
    {
        //If file does not exist then just send a 404 error
        send_404(client);
        return;
    }

    fseek(fp, 0L, SEEK_END);
    size_t cl = ftell(fp);  //Store the file's size in bytes
    rewind(fp);

    //Get the file's content type with our function
    const char *ct = get_content_type(full_path);

#define BSIZE 1024
    char buffer[BSIZE]; //Buffer that will store our response headers loaded part by part

    sprintf(buffer, "HTTP/1.1 200 OK\r\n");
    send(client->socket, buffer, strlen(buffer), 0);

    sprintf(buffer, "Connection: close\r\n");
    send(client->socket, buffer, strlen(buffer), 0);

    sprintf(buffer, "Content-Length: %u\r\n", cl);
    send(client->socket, buffer, strlen(buffer), 0);

    sprintf(buffer, "Content-Type: %s\r\n", ct);
    send(client->socket, buffer, strlen(buffer), 0);

    sprintf(buffer, "\r\n");    //BLANK LINE REPRESENTS END OF HEADER IN HTTP RESPONSE
    send(client->socket, buffer, strlen(buffer), 0);

    int r = fread(buffer, 1, BSIZE, fp);    //r stores the number of bytes read into the buffer
    while(r)
    {
        send(client->socket, buffer, r, 0);
        r = fread(buffer, 1, BSIZE, fp);
    }

    fclose(fp);
    drop_client(client);
}


SOCKET create_socket(const char *host, const char *port)
{
    printf("Configuring local address...\n");
    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    struct addrinfo *bind_address;
    getaddrinfo(host, port, &hints, &bind_address);

    printf("Creating socket...\n");
    SOCKET socket_listen;
    socket_listen = socket(bind_address->ai_family, bind_address->ai_socktype, bind_address->ai_protocol);

    if(!ISVALIDSOCKET(socket_listen))
    {
        fprintf(stderr, "socket() failed (%d) \n", GETSOCKETERRNO());
        exit(1);
    }

    printf("Binding socket to the local address...\n");
    if(bind(socket_listen, bind_address->ai_addr, bind_address->ai_addrlen))
    {
        fprintf(stderr, "bind() failed (%d)\n", GETSOCKETERRNO());
        exit(1);
    }
    freeaddrinfo(bind_address);

    printf("Listening for connections...\n");
    if(listen(socket_listen, 10) < 0)
    {
        fprintf(stderr, "listen() failed (%d) \n", GETSOCKETERRNO());
        exit(1);
    }

    return socket_listen;
}
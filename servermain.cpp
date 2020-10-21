#include <stdio.h>
#include <stdlib.h>
#include <unistd.h> 
#include <errno.h> 
#include <string.h> 
#include <netdb.h> 
#include <sys/types.h> 
#include <netinet/in.h> 
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <sys/wait.h>

#define TCP_PORT "33718"    //TCP port(with client): 33718
#define UDP_PORT "32718"    //UDP port(with server): 32718 
#define HOST "localhost"
#define BACKLOG 10 // how many pending connections queue will hold
#define PORTA "21859"
#define PORTB "22859"
#define PORTC "23859"

//Main server will ask each of Backend servers which
//countries they are responsible for.
//Main server will construct a data structure to book-keep such information


//start main server
void ask_country() 
{
    int status;
    struct addrinfo hints;
    struct addrinfo *servinfo; // 將指向結果

    memset(&hints, 0, sizeof hints); // 確保 struct 為空
    hints.ai_family = AF_UNSPEC; // 不用管是 IPv4 或 IPv6
    hints.ai_socktype = SOCK_STREAM; // TCP stream sockets
    hints.ai_flags = AI_PASSIVE; // 幫我填好我的 IP 

    if ((status = getaddrinfo(NULL, "3490", &hints, &servinfo)) != 0) {
    fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
    exit(1);
    }

    // servinfo 目前指向一個或多個 struct addrinfos 的鏈結串列

    // ... 做每件事情，一直到你不再需要 servinfo  ....

    freeaddrinfo(servinfo); // 釋放這個鏈結串列
}
//listen to clients

//call serverA and serverB

int main(int argc, char *argv[]) 
{
    start_server();
    ask_country();
    process_query();
}
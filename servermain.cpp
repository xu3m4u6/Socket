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
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <map>
#include <iterator>
using namespace std;

// Constants
#define TCP_PORT_MAIN "33718"    //TCP port(with client): 33718
#define UDP_PORT_MAIN "32718"    //UDP port(with server): 32718 
#define UDP_PORT_A "30718"
#define UDP_PORT_B "31718"
#define HOSTNAME "127.0.0.1" // server address
#define BACKLOG 10 // how many pending connections queue will hold
#define MAXBUFLEN 1000

// global variables
int sockfd_UDP;
struct addrinfo hints, *serverMainInfo, *serverAInfo, hints2;
// a map for mapping country list


// get port, IPv4 or IPv6:
in_port_t get_in_port(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET)
        return (((struct sockaddr_in*)sa)->sin_port);

    return (((struct sockaddr_in6*)sa)->sin6_port);
}

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
  if (sa->sa_family == AF_INET) {
    return &(((struct sockaddr_in*)sa)->sin_addr);
  }
  return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

void start_server_UDP()
{
    int status;
    int numbytes;
    struct sockaddr_storage their_addr;
    char buf[MAXBUFLEN];
    socklen_t addr_len;
    char s[INET6_ADDRSTRLEN];

    memset(&hints, 0, sizeof hints);

    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE; // 使用我的 IP
    
    // getaddr info
    if ((status = getaddrinfo(HOSTNAME, UDP_PORT_MAIN, &hints, &serverMainInfo)) != 0) 
    {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
        exit(0);
    }

    // create UDP socket
    if ((sockfd_UDP = socket(serverMainInfo->ai_family, serverMainInfo->ai_socktype,
    serverMainInfo->ai_protocol)) == -1) {
        perror("serverMain: fail to create socket");
    }
    // bind
    if (bind(sockfd_UDP, serverMainInfo->ai_addr, serverMainInfo->ai_addrlen) == -1) {
        close(sockfd_UDP);
        perror("serverMain: binding failed");
    }

}

void get_countrylist_from_A() 
{
    int status;
    int numbytes;
    struct sockaddr_storage their_addr;
    char buf[MAXBUFLEN];
    socklen_t addr_len;
    char s[INET6_ADDRSTRLEN];




    // ?????????????????????????????????
    memset(&hints2, 0, sizeof hints2);
    hints2.ai_family = AF_UNSPEC;
    hints2.ai_socktype = SOCK_DGRAM;

    // getaddr info
    if ((status = getaddrinfo(HOSTNAME, UDP_PORT_A, &hints2, &serverAInfo)) != 0) 
    {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
        exit(0);
    }

    // requesting server A's country list 
    string msg = "waiting for country list";

    if ((numbytes = sendto(sockfd_UDP, msg.c_str(), MAXBUFLEN, 0,
        serverAInfo->ai_addr, serverAInfo->ai_addrlen)) == -1) {
        perror("servermain: sendto ??????");
        exit(1);
    }
    printf("servermain: sent %d bytes to %s\n", numbytes, UDP_PORT_A);//

    cout << "The servermain has sent a request for country list to ServerA" << endl;//
    

    

    // receiving
    printf("serverMain: waiting to recvfrom...\n");
    addr_len = sizeof their_addr;
    if ((numbytes = recvfrom(sockfd_UDP, buf, MAXBUFLEN-1 , 0, 
            (struct sockaddr *)&their_addr, &addr_len)) == -1)
    {
        perror("recvfrom");
        exit(1);
    }
    
    // print received info
    printf("serverMain: got packet from %s port %d\n",
    inet_ntop(their_addr.ss_family,
    get_in_addr((struct sockaddr *)&their_addr), s, sizeof s), 
    ntohs(get_in_port((struct sockaddr *)&their_addr)));
    printf("serverMain: packet is %d bytes long\n", numbytes);
    buf[numbytes] = '\0';
    printf("serverMain: packet contains \"%s\"\n", buf);

    // store countryList into 
    vector<string> result; 
    istringstream iss(buf); 
    for(string buf; iss >> buf; ) 
        result.push_back(buf);

    
}

int main(int argc, char *argv[]) 
{
    start_server_UDP();
    get_countrylist();
    
    // process_query();
    
    // freeaddr info
    freeaddrinfo(serverMainInfo);
    // close socket
    close(sockfd_UDP);
}
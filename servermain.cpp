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
#include <iomanip>
#include <signal.h>
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
struct addrinfo hints, *serverMainInfo, *serverAInfo, *serverBInfo;
map<string,int> countryMap;// Mapping country to corresponding backend server


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
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;
    
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

    // getaddr info of serverA
    if ((status = getaddrinfo(HOSTNAME, UDP_PORT_A, &hints, &serverAInfo)) != 0) 
    {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
        exit(0);
    }
    // getaddr info of serverB
    if ((status = getaddrinfo(HOSTNAME, UDP_PORT_B, &hints, &serverBInfo)) != 0) 
    {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
        exit(0);
    }
}

void get_countrylist(int serverID) 
{
    int status;
    int numbytes;
    struct sockaddr_storage their_addr;
    char buf[MAXBUFLEN];
    socklen_t addr_len;
    char s[INET6_ADDRSTRLEN];

    // requesting server's country list 
    string msg = "waiting for country list";

    if(serverID == 0){
        if ((numbytes = sendto(sockfd_UDP, msg.c_str(), MAXBUFLEN, 0,
            serverAInfo->ai_addr, serverAInfo->ai_addrlen)) == -1) {
            perror("servermain: fail to send request to serverA");
            exit(1);
        }
        printf("servermain: sent %d bytes to %s\n", numbytes, UDP_PORT_A);//

        cout << "The servermain has sent a request for country list to ServerA" << endl;//
    }else{
        if ((numbytes = sendto(sockfd_UDP, msg.c_str(), MAXBUFLEN, 0,
            serverBInfo->ai_addr, serverBInfo->ai_addrlen)) == -1) {
            perror("servermain: fail to send request to serverB");
            exit(1);
        }
        printf("servermain: sent %d bytes to %s\n", numbytes, UDP_PORT_A);//

        cout << "The servermain has sent a request for country list to ServerB" << endl;//
    }
    
    // receiving country list
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
    ntohs(get_in_port((struct sockaddr *)&their_addr)));//
    printf("serverMain: packet is %d bytes long\n", numbytes);//
    buf[numbytes] = '\0';
    printf("serverMain: packet contains \"%s\"\n", buf);//

    cout << "The Main server has received the country list from server ";
    if(serverID == 0){
        cout << "A using UDP over port <" << UDP_PORT_MAIN << ">" << endl;
    }else{
        cout << "B using UDP over port <" << UDP_PORT_MAIN << ">" << endl;
    }

    // store countryList into countryMap
    istringstream iss(buf); 
    for(string countryName; iss >> countryName; ) 
        countryMap[countryName] = serverID;
}

void print_countryMap()
{   
    vector<string> serverA;
    vector<string> serverB;
    int max;
    const char separator = ' ';
    const int nameWidth = 30;
    cout << left << setw(nameWidth) << setfill(separator) << "Server A" << "|Server B" << endl;
    map<string,int>::const_iterator itr;
    for (itr = countryMap.begin(); itr != countryMap.end(); ++itr){
        if(itr->second == 0){
            serverA.push_back(itr->first);
        }else{
            serverB.push_back(itr->first);
        }
    }
    if(serverA.size() > serverB.size()){
        max = serverA.size(); 
    }else{
        max = serverB.size();
    }
    
    for(int i = 0; i < max; i++){
        if(i < serverA.size() && i < serverB.size()){
            cout << left << setw(nameWidth) << setfill(separator) << serverA[i] << "|" << serverB[i]<< endl;
        }else if(i > serverA.size()-1){
            cout << "                              " << "|" << serverB[i]<< endl;    
        }else if(i > serverB.size()-1){
            cout << left << setw(nameWidth) << setfill(separator) << serverA[i] << "|" << endl;    
        }
    } 

}

void sigchld_handler(int s)
{
  while(waitpid(-1, NULL, WNOHANG) > 0);
}

int process_query()
{
    //decide A or B //not found
    //send udp to serverA or serverB
    //receive query result from serverA or B
    //return 
}

void listen_to_clients(int sockfd)
{
    socklen_t sin_size;
    struct sockaddr_storage their_addr; // 連線者的位址資訊 
    int new_fd;
    char s[INET6_ADDRSTRLEN];
    char buf[MAXBUFLEN];

    while(1) { // 主要的 accept() 迴圈
        sin_size = sizeof their_addr;
        new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
        if (new_fd == -1) {
            perror("accept");
            continue;
        }

        inet_ntop(their_addr.ss_family,
        get_in_addr((struct sockaddr *)&their_addr), s, sizeof s);
        printf("server: got connection from %s\n", s);

        if (!fork()) { // 這個是 child process
            close(sockfd); // child 不需要 listener
            if (recv(new_fd, buf, MAXBUFLEN-1 , 0) == -1)
            {
                perror("recv");
            }

            // send msg to serverA or serverB
            // get result from serverA or serverB
            int result = process_query();
            
            if (send(new_fd, &result, , 0) == -1) // to be finished
            {
                perror("send");
            }
            close(new_fd);
            exit(0);
        }
        close(new_fd); // parent 不需要這個
    }
}

void start_server_TCP()
{
    int sockfd, new_fd; // 在 sock_fd 進行 listen，new_fd 是新的連線
    struct addrinfo hints, *servinfo;
    struct sigaction sa;
    int yes=1;
    int rv;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if ((rv = getaddrinfo(HOSTNAME, TCP_PORT_MAIN, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
    }

    // bind
    if ((sockfd = socket(servinfo->ai_family, servinfo->ai_socktype,
        servinfo->ai_protocol)) == -1) {
        perror("server: socket");
    }

    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,
        sizeof(int)) == -1) {
        perror("setsockopt");
        exit(1);
    }

    if (bind(sockfd, servinfo->ai_addr, servinfo->ai_addrlen) == -1) {
        close(sockfd);
        perror("server: bind");
    }

    freeaddrinfo(servinfo); // 全部都用這個 structure

    if (listen(sockfd, BACKLOG) == -1) {
        perror("listen");
        exit(1);
    }

    sa.sa_handler = sigchld_handler; // 收拾全部死掉的 processes
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;

    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        perror("sigaction");
        exit(1);
    }

    printf("server: waiting for connections...\n");

    listen_to_clients(sockfd);
}

int main(int argc, char *argv[]) 
{
    start_server_UDP();

    get_countrylist(0); //get country list from server A
    get_countrylist(1); //get country list from server B
    print_countryMap();

    start_server_TCP();

    // freeaddr info for UDP
    freeaddrinfo(serverMainInfo);
    freeaddrinfo(serverAInfo);
    freeaddrinfo(serverBInfo);

    // close socket
    close(sockfd_UDP);
}
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
#define TCP_PORT_MAIN "33718" //TCP port(with client): 33718
#define UDP_PORT_MAIN "32718" //UDP port(with server): 32718 
#define UDP_PORT_A "30718" //serverA UDP port
#define UDP_PORT_B "31718" //serverB UDP port
#define HOSTNAME "127.0.0.1" // localhost IP address
#define BACKLOG 10 // how many pending connections queue will hold
#define MAXBUFLEN 1000

// global variables
int sockfd_UDP;
struct addrinfo hints, *serverMainInfo, *serverAInfo, *serverBInfo;
map<string,int> countryMap;// Mapping country to corresponding backend server


// get port, IPv4 or IPv6: (beej)
in_port_t get_in_port(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET)
        return (((struct sockaddr_in*)sa)->sin_port);

    return (((struct sockaddr_in6*)sa)->sin6_port);
}

// get sockaddr, IPv4 or IPv6: (beej)
void *get_in_addr(struct sockaddr *sa)
{
  if (sa->sa_family == AF_INET) {
    return &(((struct sockaddr_in*)sa)->sin_addr);
  }
  return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

// create main server UDP socket
void start_server_UDP()
{
    int status;
    memset(&hints, 0, sizeof hints); //(beej)
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;
    
    // getaddr info (beej)
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
    cout << "The Main Server is up and running." << endl;
}

// send country list request to backend server and receive country list
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
    }else{
        if ((numbytes = sendto(sockfd_UDP, msg.c_str(), MAXBUFLEN, 0,
            serverBInfo->ai_addr, serverBInfo->ai_addrlen)) == -1) {
            perror("servermain: fail to send request to serverB");
            exit(1);
        }
    }
    
    // receiving country list
    addr_len = sizeof their_addr;
    if ((numbytes = recvfrom(sockfd_UDP, buf, MAXBUFLEN-1 , 0, 
            (struct sockaddr *)&their_addr, &addr_len)) == -1)
    {
        perror("servermain: fail to recv country list");
        exit(1);
    }
    buf[numbytes] = '\0';

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

// print country mapping on screen
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

// handle finished processes
void sigchld_handler(int s)
{
  while(waitpid(-1, NULL, WNOHANG) > 0);
}

// main server send query request to backend and rcv result
string send_query_and_rcv_result(string id, string country, int serverID)
{
    int status;
    int numbytes;
    struct sockaddr_storage their_addr;
    char buf[MAXBUFLEN];
    socklen_t addr_len;
    char s[INET6_ADDRSTRLEN];

    // send query request to backend server
    string msg = id + " " + country;

    if(serverID == 0){
        if ((numbytes = sendto(sockfd_UDP, msg.c_str(), MAXBUFLEN, 0,
            serverAInfo->ai_addr, serverAInfo->ai_addrlen)) == -1) {
            perror("servermain: fail to send request to serverA");
            exit(1);
        }

        cout << "The Main Server has sent a request from User<" << id ;
        cout << "> to server A using UDP over port<" << UDP_PORT_MAIN << ">" << endl;
    }else{
        if ((numbytes = sendto(sockfd_UDP, msg.c_str(), MAXBUFLEN, 0,
            serverBInfo->ai_addr, serverBInfo->ai_addrlen)) == -1) {
            perror("servermain: fail to send request to serverB");
            exit(1);
        }

        cout << "The Main Server has sent a request from User<" << id ;
        cout << "> to server B using UDP over port<" << UDP_PORT_MAIN << ">" << endl;
    }
    
    // receiving query result
    addr_len = sizeof their_addr;
    if ((numbytes = recvfrom(sockfd_UDP, buf, MAXBUFLEN-1 , 0, 
            (struct sockaddr *)&their_addr, &addr_len)) == -1)
    {
        perror("recvfrom");
        exit(1);
    }
    buf[numbytes] = '\0';

    // print result received from backend server
    istringstream iss(buf); 
    string result;
    iss >> result;
    if(result.compare("USER_NOT_FOUND") == 0) {
        cout << "The Main server has received \"User ID: Not found\" from server ";
    }else{
        cout << "The Main server has received searching result of User<" << id << "> from server";
    }
    if(serverID == 0){
        cout << "<A>" << endl;
    }else {
        cout << "<B>" << endl;
    }

    return result;
}

// process the request from client and decide which backend server to forward request
string process_query(char *buf)
{
    // get id and country from input
    istringstream iss(buf); 
    string id, country;
    iss >> id; 
    iss >> country;
    cout << "The Main server has received the request on User<" << id << "> in <";
    cout << country << "> from the client using TCP over port " << TCP_PORT_MAIN << endl;

    //decide send to server A/B or country not found
    map<string,int>::iterator itr;
    itr = countryMap.find(country);
    if(itr != countryMap.end()){
        cout << "<" << country << "> shows up in server ";
        if(itr->second == 0){
            cout << "A" << endl;
        }else{
            cout << "B" << endl;
        }
        return send_query_and_rcv_result(id, country, itr->second); //send request through udp to serverA or serverB
    }
    else{
        cout << "<" << country << "> does not show up in server A&B" << endl;
        return "COUNTRY_NOT_FOUND";
    }
}

// listen to clients query requests
void listen_to_clients(int sockfd_TCP)
{
    socklen_t sin_size;
    struct sockaddr_storage their_addr; // client info
    int new_fd; //child socket
    char s[INET6_ADDRSTRLEN];
    char buf[MAXBUFLEN];

    // (beej)
    while(1) { // accept() loop 
        sin_size = sizeof their_addr;
        new_fd = accept(sockfd_TCP, (struct sockaddr *)&their_addr, &sin_size);
        if (new_fd == -1) {
            perror("servermain: fail to accept");
            continue;
        }

        if (!fork()) { // child process
            close(sockfd_TCP);
            if (recv(new_fd, buf, MAXBUFLEN-1 , 0) == -1)
            {
                perror("servermain: child socket fail to recv");
            }
            
            // process query request and get the result
            string result = process_query(buf);

            // send result back to client
            if (send(new_fd, result.c_str(), result.length()+1, 0) == -1) 
            {
                perror("servermain: fail to send query result to client");
            }

            if(result.compare("COUNTRY_NOT_FOUND") == 0) {
                cout << "The Main Server has sent \"Country Name: Not found\" to the client using TCP over port <" << TCP_PORT_MAIN << ">" << endl;
            } else if (result.compare("USER_NOT_FOUND") == 0) {
                cout << "The Main Server has sent error to the client using TCP over port <" << TCP_PORT_MAIN << ">" << endl;
            } else {
                cout << "The Main Server has sent searching result to the client using TCP over port <" << TCP_PORT_MAIN << ">" << endl;
            }

            close(new_fd);
            exit(0);
        }
        close(new_fd);
    }
}

// create mainserver TCP socket and call listen to client
void start_server_TCP()
{
    int sockfd_TCP; // TCP socket
    struct addrinfo hints, *servinfo;
    struct sigaction sa;
    int yes=1;
    int rv;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;//

    if ((rv = getaddrinfo(NULL, TCP_PORT_MAIN, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
    }

    // create
    if ((sockfd_TCP = socket(servinfo->ai_family, servinfo->ai_socktype,
        servinfo->ai_protocol)) == -1) {
        perror("servermain: failed to create TCP socket");
    }

    if (setsockopt(sockfd_TCP, SOL_SOCKET, SO_REUSEADDR, &yes,
        sizeof(int)) == -1) {
        perror("servermain: fail to setsockopt");
        exit(1);
    }
    // bind
    if (bind(sockfd_TCP, servinfo->ai_addr, servinfo->ai_addrlen) == -1) {
        close(sockfd_TCP);
        perror("servermain: TCP fail to bind");
    }

    freeaddrinfo(servinfo);

    // listen to client
    if (listen(sockfd_TCP, BACKLOG) == -1) {
        perror("servermain: fail to listen to client");
        exit(1);
    }

    sa.sa_handler = sigchld_handler; // kill all finished processes
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;

    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        perror("servermain: sigaction failed");
        exit(1);
    }
    
    listen_to_clients(sockfd_TCP); // listen to clients query request
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

    close(sockfd_UDP); // close socket
}
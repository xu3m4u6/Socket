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
#include <iostream>
#include <string> 
#include <iomanip>
#include <sstream>
using namespace std;


#define MAXDATASIZE 100 // Maximum data size we can receive at one time（number of bytes）
#define TCP_PORT_MAIN "33718" //server Main TCP port 33718
#define HOSTNAME "127.0.0.1" // localhost IP address


// get IPv4 or IPv6 sockaddr (beej)
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int main(int argc, char *argv[])
{
    int sockfd, numbytes;
    char buf[MAXDATASIZE];
    struct addrinfo hints, *serverMainInfo;
    int rv;
    char s[INET6_ADDRSTRLEN];
    
    cout << "The client is up and running" << endl;
    while(1)
    {
        // (beej)
        memset(&hints, 0, sizeof hints);
        hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = SOCK_STREAM;

        if ((rv = getaddrinfo(HOSTNAME, TCP_PORT_MAIN, &hints, &serverMainInfo)) != 0) {
            fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
            return 1;
        }

        // create TCP socket
        if ((sockfd = socket(serverMainInfo->ai_family, serverMainInfo->ai_socktype,
            serverMainInfo->ai_protocol)) == -1) {
            perror("client: socket");
        }

        if (connect(sockfd, serverMainInfo->ai_addr, serverMainInfo->ai_addrlen) == -1) {
            close(sockfd);
            perror("client: connect");
        }

        inet_ntop(serverMainInfo->ai_family, get_in_addr((struct sockaddr *)serverMainInfo->ai_addr), s, sizeof s);
        freeaddrinfo(serverMainInfo);
        
        // read input information
        string id;
        string country;
        cout << "Please enter the User ID: ";
        cin >> id;
        cout << "Please enter the Country Name: ";
        cin >> country;

        // send request
        string msg;
        msg = id + " " + country;
        if (send(sockfd, msg.c_str(), MAXDATASIZE, 0) == -1)
        {
            perror("send");
        }
        cout << "The client has sent User<" << id;
        cout << "> and <" << country << "> to Main Server using TCP" << endl;

        // client receive from main server
        if ((numbytes = recv(sockfd, buf, MAXDATASIZE-1, 0)) == -1) //
        {
            perror("recv");
            exit(1);
        }
        buf[numbytes] = '\0';

        // process & print result
        string result;
        stringstream ss(buf);
        ss >> result;
        if(result.compare("COUNTRY_NOT_FOUND") == 0) {
            cout << "<" << country << "> not found " << endl;
        } else if (result.compare("USER_NOT_FOUND") == 0) {
            cout << "User " << id << " not found " << endl;
        } else {
            cout << "The client has received results from Main Server: User<";
            cout << result << "> is possible friend of User<" ;
            cout << id << "> in <" << country << ">" << endl;
        }

        cout << "-----Start a new request-----" << endl;

    }
    close(sockfd);
}
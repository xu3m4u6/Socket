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


#define MAXDATASIZE 100 // 我們一次可以收到的最大位原組數（number of bytes）
#define TCP_PORT_MAIN "33718" //TCP port(with client): 33718
#define HOSTNAME "127.0.0.1" // server address


// 取得 IPv4 或 IPv6 的 sockaddr：
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
    struct addrinfo hints, *servinfo;
    int rv;
    char s[INET6_ADDRSTRLEN];
    
    printf("The client is up and running");
    while(1)
    {

        memset(&hints, 0, sizeof hints);
        hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = SOCK_STREAM;

        if ((rv = getaddrinfo(HOSTNAME, TCP_PORT_MAIN, &hints, &servinfo)) != 0) {
            fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
            return 1;
        }

        // create TCP socket
        if ((sockfd = socket(servinfo->ai_family, servinfo->ai_socktype,
            servinfo->ai_protocol)) == -1) {
            perror("client: socket");
        }

        if (connect(sockfd, servinfo->ai_addr, servinfo->ai_addrlen) == -1) {
            close(sockfd);
            perror("client: connect");
        }

        inet_ntop(servinfo->ai_family, get_in_addr((struct sockaddr *)servinfo->ai_addr), s, sizeof s);

        printf("client: connecting to %s\n", s);//
        freeaddrinfo(servinfo);
        
        // read input information
        string id;
        string country;
        cout << "Please enter the User ID: ";
        cin >> id;
        cout << "Please enter the Country Name: ";
        cin >> country;

        string msg;
        msg = id + " " + country;
        if (send(sockfd, msg.c_str(), MAXDATASIZE, 0) == -1)
        {
            perror("send");
        }
        
        cout << "The client has sent User ";
        cout << id << "> in <" << country << "> to Main Server using TCP\n" << endl;
        // printf("The client has sent User'%s' and '%s' to Main Server using TCP\n", id, country.c_str());


        if ((numbytes = recv(sockfd, buf, MAXDATASIZE-1, 0)) == -1) //
        {
            perror("recv");
            exit(1);
        }
        cout << "numbytes= " << numbytes << endl;
        buf[numbytes] = '\0';
        printf("client: received '%s'\n", buf);//
        cout << buf <<" end of buf" <<endl;

        string result;
        istringstream iss(buf);
        iss >> result;
        cout << "client: received with: " << result << endl;//
        if(result == "COUNTRY_NOT_FOUND") {
            cout << country << " not found " << endl;
        } else if (result == "USER_NOT_FOUND") {
            cout << id << " not found " << endl;
        } else {
            cout << "The client has received results from Main Server: User<" << buf << "> is possible friend of User<" ;
            cout << id << "> in <" << country << ">" << endl;
        }

        // printf("The client has received results from Main Server:User'%s' is possible friend of User'%s' in '%s'\n", buf, id.c_str(), country.c_str());
    }
    close(sockfd);
}
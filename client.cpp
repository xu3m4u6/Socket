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

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if ((rv = getaddrinfo(HOSTNAME, TCP_PORT_MAIN, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    // 用迴圈取得全部的結果，並先連線到能成功連線的
    if ((sockfd = socket(servinfo->ai_family, servinfo->ai_socktype,
        servinfo->ai_protocol)) == -1) {
        perror("client: socket");
    }

    if (connect(sockfd, servinfo->ai_addr, servinfo->ai_addrlen) == -1) {
        close(sockfd);
        perror("client: connect");
    }

    inet_ntop(servinfo->ai_family, get_in_addr((struct sockaddr *)servinfo->ai_addr), s, sizeof s);

    printf("client: connecting to %s\n", s);
    printf("The client is up and running");

    freeaddrinfo(servinfo); // 全部皆以這個 structure 完成

    while(1)
    {
        int id;
        string country;
        cout << "Please enter the User ID: ";
        cin >> id;
        cout << "Please enter the Country Name: ";
        cin >> country;


        if (send(sockfd, &id, , 0) == -1)
        {
            perror("send");
        }
        
        printf("Client2 has sent User'%d' and '%s' to Main Server using TCP\n", id, country);

        sleep(2000);

        if ((numbytes = recv(sockfd, buf, MAXDATASIZE-1, 0)) == -1) 
        {
            perror("recv");
            exit(1);
        }
        buf[numbytes] = '\0';
        printf("client: received '%s'\n",buf);
        printf("Client2 has received results from Main Server:User<user ID1>, User<user ID2> is/are possible friend(s) of User<user ID> in <Country Name>\n", id, country);
    }
    close(sockfd);
}
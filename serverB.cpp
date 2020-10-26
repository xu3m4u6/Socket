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

using namespace std;


// Constants
#define HOSTNAME "127.0.0.1" // server address
#define UDP_PORT_MAIN "32718"
#define UDP_PORT_B "31718" //server B port number
#define MAXBUFLEN 1000

// global variables
vector<vector<vector<int> > > graphs; //keep the connection graph of every country
vector<string> countryList; //contains all of the country names in file
map<string,int> countryIndex; //keep country->index mapping for each country
vector<map<string, int> > userId_to_reindex; //keep userID->reindex mapping for every country
vector<map<int, string> > reindex_to_userId; //keep reindex->userID mapping for every country

int sockfd_UDP; // ServerB UDP socket
struct addrinfo hints, *serverMainInfo, *serverBInfo; //serverMainInfo 將指向結果



// create the graph of one country
void create_graph(vector< vector<string> > allusers){
    // cout << "-------------create graph-------------" << endl;//
    int index = 0;
    vector<vector<int> > adjacent_matrix(allusers.size(), vector<int> (allusers.size(), 0));
    map<string, int> user_reindex; //map userId to reindex index
    map<int, string> user_original_id; //map reindex index to userId

    // keep record of the relationship between original userId and reindex index
    for(int i = 0; i < adjacent_matrix.size(); i++){
        string userId = allusers[i][0];
        user_reindex[userId] = index;
        user_original_id[index] = userId;
        index++;
	}
    userId_to_reindex.push_back(user_reindex);
    reindex_to_userId.push_back(user_original_id);

    // update all the connections to the adjacent matrix from allusers
    for(int i = 0; i < allusers.size(); i++){
        int currUser = user_reindex.find(allusers[i][0])->second;
        for(int j = 0; j < allusers[i].size(); j++){
            if(j != 0){
                int friendId = user_reindex.find(allusers[i][j])->second;
                adjacent_matrix[currUser][friendId] = 1;
                adjacent_matrix[friendId][currUser] = 1;
            }
        }
	}
    graphs.push_back(adjacent_matrix);
}

//read file and construct a vector of “graphs” 
void read_file() 
{
    // Read from the text file
    ifstream inFile;
    inFile.open("data2.txt");
    if(!inFile){
        cerr << "Can't open data2.txt" << endl;
        exit(0);
    }

    string line;
	vector< vector<string> > users;
    int countryCount = 0;
    bool firstCountry = true;

	while (getline(inFile,line))
	{
        if(!isdigit(line.at(0))){
            if(!firstCountry){
                create_graph(users);
                users.clear();
            }
            firstCountry = false;
            countryList.push_back(line);
            countryIndex[line]=countryCount;
            countryCount++;
        }
        else{
            stringstream ss(line);
		    string friendId;
            vector<string> oneUser; //keep all the connection for one user

            while(ss >> friendId){
                oneUser.push_back(friendId);
            }
            users.push_back(oneUser);
        }
	}
    create_graph(users);

    inFile.close();
}

void start_serverB(){
    int status;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;

    // mainserver info for send
    if ((status = getaddrinfo(HOSTNAME, UDP_PORT_MAIN, &hints, &serverMainInfo)) != 0) 
    {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
        exit(0);
    }

    // serverA info for bind
    if ((status = getaddrinfo(HOSTNAME, UDP_PORT_B, &hints, &serverBInfo)) != 0) 
    {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
        exit(0);
    }

    // create UDP socket
    if ((sockfd_UDP = socket(serverBInfo->ai_family, serverBInfo->ai_socktype, serverBInfo->ai_protocol)) == -1) {
        perror("serverA: fail to create socket");
    }
    // bind
    if (bind(sockfd_UDP, serverBInfo->ai_addr, serverBInfo->ai_addrlen) == -1) {
        close(sockfd_UDP);
        perror("serverA: binding failed");
    }
    cout << "The server B is up and running using UDP on port <31718>" << endl;
}

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

void send_countrylist() {
    int numbytes;
    char buf[MAXBUFLEN];
    struct sockaddr_storage their_addr;
    socklen_t addr_len;
    char s[INET6_ADDRSTRLEN];

    // receiving
    printf("serverB: waiting for servermain to startup ...\n");//
    addr_len = sizeof their_addr;
    if ((numbytes = recvfrom(sockfd_UDP, buf, MAXBUFLEN-1 , 0, 
            (struct sockaddr *)&their_addr, &addr_len)) == -1)
    {
        perror("recvfrom");
        exit(1);
    }
    
    // print received info
    printf("serverB: got packet from %s port %d\n",
    inet_ntop(their_addr.ss_family,
    get_in_addr((struct sockaddr *)&their_addr), s, sizeof s), 
    ntohs(get_in_port((struct sockaddr *)&their_addr)));
    printf("serverB: packet is %d bytes long\n", numbytes);//
    buf[numbytes] = '\0';
    printf("serverB: packet contains \"%s\"\n", buf);//

    // send msg
    string msg;
    for(int j = 0; j < countryList.size(); j++){
        msg = msg + " " + countryList[j];
    }
    if ((numbytes = sendto(sockfd_UDP, msg.c_str(), MAXBUFLEN, 0,
        serverMainInfo->ai_addr, serverMainInfo->ai_addrlen)) == -1) {
        perror("serverB: sendto");
        exit(1);
    }
    printf("serverB: sent %d bytes to %s\n", numbytes, UDP_PORT_MAIN);//

    cout << "The server B has sent a country list to Main Server" << endl;
    
}

string query(string userId, string countryName){
    cout << "-------------query-------------" << endl;//
    // get the index of the country in countryList
    int countryIndex = -1;
    for(int i = 0; i < countryList.size(); i++){
        if(countryList[i] == countryName){
            countryIndex = i;
            break;
        }
    }
    // cout << "country index is " << countryIndex << endl;//
    
    // check if user exist in the country
    int target_user_index = -1;
    if(userId_to_reindex[countryIndex].count(userId) > 0){
        target_user_index = userId_to_reindex[countryIndex].at(userId);
        // cout << "user " << userId << " exist, reindex: " << target_user_index << endl;//
    }else{
        cout << "User<" << userId << "> does not show up in <" << countryName << ">" << endl;
        return "USER_NOT_FOUND";
    }

    // recommend
    cout << "The server B is searching possible friends for User<" << userId << "> ..." << endl;
    int usersCount = userId_to_reindex[countryIndex].size(); //the number of the users in the country
    map<int, int> unconnected; //mapping unconnected User's reindex -> number of common friends

    // check if user is the only user
    if(usersCount == 1){ return "NONE"; }
    
    // check if target user is connected to all other user, if not,
    // find unconnected user's reindex and the number of common friends
    int target_user_friend_count = 0;
    for(int i = 0; i < graphs[countryIndex][target_user_index].size(); i++){
        if(graphs[countryIndex][target_user_index][i] == 1){
            target_user_friend_count++;
        }else{
            if(target_user_index != i){
                int commonFriendCount = 0; // count common friends
                for(int j = 0; j < usersCount; j++){
                    if(graphs[countryIndex][target_user_index][j] == 1 && graphs[countryIndex][i][j] == 1 ){
                        commonFriendCount++;
                    }
                }
                unconnected[i] = commonFriendCount;
            }
        }
    }

    //target user is connected to all the other users
    if(target_user_friend_count == usersCount-1){ return "NONE"; }

    // find the max common friend count, deal with same situation
    map<int, int>::const_iterator max_itr;
    int max_common_friend_count = 0;
    int max_common_userIndex = -1; //index of the user who has max common friend with target
    for (max_itr = unconnected.begin(); max_itr != unconnected.end(); ++max_itr){
        // cout << "unconnected User index: " << max_itr->first;//
        // cout << " => common friend count " << max_itr->second << endl;//
        
        // if user has more common friend than current max
        if(max_itr->second > max_common_friend_count){
            max_common_friend_count = max_itr->second;
            max_common_userIndex = max_itr->first;
            // cout << "current max count= " << max_common_friend_count;//
            // cout << " from user index= " << max_itr->first << endl;//
        }else if(max_itr->second == max_common_friend_count && max_itr->second != 0){
            // cout << max_itr->first<<endl;
            int curr_smallest_id = atoi(reindex_to_userId[countryIndex].at(max_common_userIndex).c_str());
            int comparing_id = atoi(reindex_to_userId[countryIndex].at(max_itr->first).c_str());
            // cout << "curr_smallest_id:" << curr_smallest_id;//
            // cout << " comparing_id:" << comparing_id << endl;//
            if(comparing_id < curr_smallest_id){
                max_common_userIndex = max_itr->first;
            }
            // cout << "current max count= " << max_common_friend_count;//
            // cout << " from user index= " << max_common_userIndex << endl;//
        }
    }
    // cout << "print out unconnected user and common friend" << endl;
    // map<int,int>::const_iterator itr;
    // for (itr = unconnected.begin(); itr != unconnected.end(); ++itr){
    //     cout << "unconnected User index: " << itr->first << " => count " << itr->second << endl;
    // }

    // have common friends with unconnected users
    if(max_common_friend_count != 0){
        cout << "have common, recommend " << max_common_userIndex << endl;//
        return reindex_to_userId[countryIndex].at(max_common_userIndex);
    }

    // no common friends with unconnected users
    int have_max_friend_index = -1;
    int max_friend_count = -1;
    for(int i = 0; i < usersCount; i++){
        int friendCount = 0;
        if(i != target_user_index){
            for(int j = 0; j < usersCount; j++){
                if(graphs[countryIndex][i][j] == 1){
                    friendCount++;
                }
            }
            if(max_friend_count == friendCount){
                int curr_smallest_id = atoi(reindex_to_userId[countryIndex].at(max_friend_count).c_str());
                int comparing_id = atoi(reindex_to_userId[countryIndex].at(i).c_str());
                // cout << "curr_smallest_id:" << curr_smallest_id;//
                // cout << " comparing_id:" << comparing_id << endl;//
                if(comparing_id < curr_smallest_id){
                    have_max_friend_index = i;
                }
                // cout << "current max count= " << max_friend_count;//
                // cout << " from user index= " << have_max_friend_index << endl;//
            }else if(max_friend_count < friendCount){
                max_friend_count = friendCount;
                have_max_friend_index = i;
            }
        }
    }
    cout << "no common, recommend " << have_max_friend_index << endl;//
    return reindex_to_userId[countryIndex].at(have_max_friend_index);
}

void listen_to_main(){
    cout << "The server B has received request for finding possible friends of User<?id?> in <?Countryname?>" << endl;
    
    // do query here?

    // int query_result = query("23", "A");
    // if(query_result == -1){
    //     cout << "The server A has sent \"User<?id?> not found\" to Main Server" << endl;
    // }

}



int main(int argc, char *argv[]) 
{
    start_serverB();
    read_file();
    
    send_countrylist();
    // listen_to_main();

    // cout << "The server B has sent the result to Main Server" << endl;

    // close socket
    freeaddrinfo(serverMainInfo);
    freeaddrinfo(serverBInfo);
    close(sockfd_UDP);
    exit(0);
}
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
#define HOSTNAME "127.0.0.1" // localhost IP address
#define UDP_PORT_MAIN "32718" //server Main UDP port number
#define UDP_PORT_A "31718" //server B UDP port number
#define MAXBUFLEN 1000

// global variables
vector<vector<vector<int> > > graphs; //keep the connection graph of every country
vector<string> countryList; //contains all of the country names in file
map<string,int> countryIndex; //keep country->index mapping for each country
vector<map<string, int> > userId_to_reindex; //keep userID->reindex mapping for every country
vector<map<int, string> > reindex_to_userId; //keep reindex->userID mapping for every country

int sockfd_UDP; // ServerB UDP socket
struct addrinfo hints, *serverMainInfo, *serverBInfo; //address info



// create the graph of one country
void create_graph(vector< vector<string> > allusers){

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
    
    // add the adjacent matrix of a new country to graph
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

    string line; //temp line for putting content reading from file
	vector< vector<string> > users; //2D vector to store all the users in one country
    int countryCount = 0; //country count in that file
    bool firstCountry = true; //indicate whether the current processing country is the first one

	while (getline(inFile,line))
	{
        if(!isdigit(line.at(0))){ //check the line is country
            if(!firstCountry){
                create_graph(users);
                users.clear();
            }
            firstCountry = false;
            countryList.push_back(line);
            countryIndex[line]=countryCount;
            countryCount++;
        }
        else{ //the line is user data
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
    memset(&hints, 0, sizeof hints); //(beej)
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;

    // mainserver info for send
    if ((status = getaddrinfo(HOSTNAME, UDP_PORT_MAIN, &hints, &serverMainInfo)) != 0) 
    {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
        exit(0);
    }

    // serverB info for bind
    if ((status = getaddrinfo(HOSTNAME, UDP_PORT_A, &hints, &serverBInfo)) != 0) 
    {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
        exit(0);
    }

    // create UDP socket
    if ((sockfd_UDP = socket(serverBInfo->ai_family, serverBInfo->ai_socktype, serverBInfo->ai_protocol)) == -1) {
        perror("server B: fail to create socket");
    }
    // bind
    if (bind(sockfd_UDP, serverBInfo->ai_addr, serverBInfo->ai_addrlen) == -1) {
        close(sockfd_UDP);
        perror("server B: binding failed");
    }
    cout << "The server B is up and running using UDP on port <" << UDP_PORT_A << ">" << endl;
}

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

void send_countrylist() {
    int numbytes;
    char buf[MAXBUFLEN];
    struct sockaddr_storage their_addr;
    socklen_t addr_len;
    char s[INET6_ADDRSTRLEN];

    // Receiving servermain's country list request
    addr_len = sizeof their_addr;
    if ((numbytes = recvfrom(sockfd_UDP, buf, MAXBUFLEN-1 , 0, 
            (struct sockaddr *)&their_addr, &addr_len)) == -1)
    {
        perror("server B: fail to recvfrom main server's country list request");
        exit(1);
    }
    buf[numbytes] = '\0';

    // send country list
    string msg;
    for(int j = 0; j < countryList.size(); j++){
        msg = msg + " " + countryList[j];
    }
    if ((numbytes = sendto(sockfd_UDP, msg.c_str(), MAXBUFLEN, 0,
        serverMainInfo->ai_addr, serverMainInfo->ai_addrlen)) == -1) {
        perror("server B: fail to send country list to main server");
        exit(1);
    }

    cout << "The server B has sent a country list to Main Server" << endl;
    
}

// return three kinds of result, each result indicates the meaning as below
// 1. USER_NOT_FOUND: userID is not exist in the country
// 2. NONE: the query cannot find anyone to recommend
// 3. recommended userID: the query find a recommendation
string query(string userId, string countryName){

    // get the index of the country in countryList
    int countryIndex = -1;
    for(int i = 0; i < countryList.size(); i++){
        if(countryList[i] == countryName){
            countryIndex = i;
            break;
        }
    }
    
    // check if user exist in the country
    int target_user_index = -1;
    if(userId_to_reindex[countryIndex].count(userId) > 0){
        target_user_index = userId_to_reindex[countryIndex].at(userId);
    }else{
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
                unconnected[i] = commonFriendCount; //add unconnected user and their common friend count
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
        // if user has more common friend than current max
        if(max_itr->second > max_common_friend_count){
            max_common_friend_count = max_itr->second;
            max_common_userIndex = max_itr->first;
        }else if(max_itr->second == max_common_friend_count && max_itr->second != 0){
            int curr_smallest_id = atoi(reindex_to_userId[countryIndex].at(max_common_userIndex).c_str());
            int comparing_id = atoi(reindex_to_userId[countryIndex].at(max_itr->first).c_str());
            if(comparing_id < curr_smallest_id){
                max_common_userIndex = max_itr->first;
            }
        }
    }

    // have common friends with unconnected users
    if(max_common_friend_count != 0){
        cout << "Here are the results: User< " << max_common_userIndex << endl;//
        return reindex_to_userId[countryIndex].at(max_common_userIndex);
    }

    // no common friends with unconnected users, find out which unconnected user has maximum friends
    int have_max_friend_index = -1;
    int max_friend_count = -1;
    map<int,int>::const_iterator itr;
    for (itr = unconnected.begin(); itr != unconnected.end(); ++itr){
        int friendCount = 0;
        // calculate the friends amount of this unconnected user
        for(int j = 0; j < usersCount; j++){
            if(graphs[countryIndex][itr->first][j] == 1){
                friendCount++;
            }
        }
        if(max_friend_count == friendCount){ //is same as the current max then compare userID
            int curr_smallest_id = atoi(reindex_to_userId[countryIndex].at(max_friend_count).c_str());
            int comparing_id = atoi(reindex_to_userId[countryIndex].at(itr->first).c_str());
            if(comparing_id < curr_smallest_id){
                have_max_friend_index = itr->first;
            }
        }else if(max_friend_count < friendCount){ //is larger than current max then update max
            max_friend_count = friendCount;
            have_max_friend_index = itr->first;
        }

    }

    return reindex_to_userId[countryIndex].at(have_max_friend_index);
}

// listen to servermain's query request and respond with result
void listen_and_respond(){

    int numbytes;
    char buf[MAXBUFLEN];
    struct sockaddr_storage their_addr;
    socklen_t addr_len;
    char s[INET6_ADDRSTRLEN];

    // receiving
    addr_len = sizeof their_addr;
    if ((numbytes = recvfrom(sockfd_UDP, buf, MAXBUFLEN-1 , 0, 
            (struct sockaddr *)&their_addr, &addr_len)) == -1)
    {
        perror("recvfrom");
        exit(1);
    }
    buf[numbytes] = '\0';

    // get request infomation
    stringstream ss(buf);
    string id;
    string country;
    ss >> id;
    ss >> country;
    cout << "The server B has received request for finding possible friends of User<" << id << "> in <" << country << ">" << endl;
    
    // process query and get the result
    string result;
    result = query(id, country);

    // send result back to server main
    if ((numbytes = sendto(sockfd_UDP, result.c_str(), result.length(), 0,
        serverMainInfo->ai_addr, serverMainInfo->ai_addrlen)) == -1) {
        perror("server B: fail to send to main with results");
        exit(1);
    }

    // print onscreen message
    if(result == "USER_NOT_FOUND"){
        cout << "User<" << id << "> does not show up in <" << country << ">" << endl;
        cout << "The server B has sent \"User<" << id << "> not found\" to Main Server" << endl;
    }else{
        cout << "Here is the result: " << result << endl;
        cout << "The server B has sent the result to Main Server" << endl;
    }
}


int main(int argc, char *argv[]) 
{
    start_serverB();
    read_file();
    send_countrylist();

    // listen to main server's request
    while(1){
        listen_and_respond();
    }

    // close socket
    freeaddrinfo(serverMainInfo);
    freeaddrinfo(serverBInfo);
    close(sockfd_UDP);
    exit(0);
}

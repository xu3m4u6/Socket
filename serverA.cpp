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
#define UDP_PORT_A "30718" //server A port number
#define MAXBUFLEN 1000

// global variables
vector<vector<vector<int> > > graphs; //keep the connection graph of every country
vector<string> countryList; //contains all of the country names in file
map<string,int> countryIndex; //keep country->index mapping for each country
vector<map<string, int> > userId_to_reindex; //keep userID->reindex mapping for every country
vector<map<int, string> > reindex_to_userId; //keep reindex->userID mapping for every country

int sockfd_UDP; // ServerA UDP socket
struct addrinfo hints, *serverMainInfo, *serverAInfo; //serverMainInfo 將指向結果



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
    // print adjacent
    // for(int i = 0; i < adjacent_matrix.size(); i++){
    //     for(int j = 0; j < adjacent_matrix[i].size(); j++){
    //         cout << adjacent_matrix[i][j] << " ";
    //     }
    //     cout << endl;
	// }

    graphs.push_back(adjacent_matrix);
}

//read file and construct a vector of “graphs” 
void read_file() 
{
    // Read from the text file
    ifstream inFile;
    inFile.open("data1.txt");
    if(!inFile){
        cerr << "Can't open data1.txt" << endl;
        exit(0);
    }

    string line;
	vector< vector<string> > users;
    int countryCount = 0;
    bool firstCountry = true;

	while (getline(inFile,line))
	{
		// cout << line << endl;
        if(!isdigit(line.at(0))){
            if(!firstCountry){
                // cout << "print user data for one country" <<endl;
                // for(int i = 0; i < users.size(); i++){
                //     for(int j = 0; j < users[i].size(); j++){
                //         // cout << users[i][j].at(0);
                //         cout << users[i].at(j) << " ";
                //     }
                //     cout << endl;
                // }
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

    // print the list of the country
    // for(int j = 0; j < countryList.size(); j++){
    //     cout << countryList[j] << " ";
    // }
    // cout << endl;
    inFile.close();

    // print graph
    // for(int i = 0; i < graphs.size(); i++){
    //     for(int j = 0; j < graphs[i].size(); j++){
    //         for(int k = 0; k < graphs[i][j].size();k++){
    //             cout << graphs[i][j][k] << " ";
    //         }
    //         cout << endl;
    //     }
    //     cout << endl;
    // }

    // print map
    // cout << "print out the index of corresponding country" << endl;
    // map<string,int>::const_iterator itr;
    // for (itr = countryIndex.begin(); itr != countryIndex.end(); ++itr){
    //     cout << itr->first << " => index " << itr->second << endl;
    // }

    // print userId to reindex & reindex to userId
    // cout << "print out userId to reindex" << endl;
    // map<string,int>::const_iterator itr;
    // for (itr = userId_to_reindex[0].begin(); itr != userId_to_reindex[0].end(); ++itr){
    //     cout << itr->first << " => index " << itr->second << endl;
    // }
    // map<int, string>::const_iterator itr2;
    // for (itr2 = reindex_to_userId[1].begin(); itr2 != reindex_to_userId[1].end(); ++itr2){
    //     cout << itr2->first << " => index " << itr2->second << endl;
    // }


}

void start_serverA(){
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
    if ((status = getaddrinfo(HOSTNAME, UDP_PORT_A, &hints, &serverAInfo)) != 0) 
    {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
        exit(0);
    }

    // create UDP socket
    if ((sockfd_UDP = socket(serverAInfo->ai_family, serverAInfo->ai_socktype, serverAInfo->ai_protocol)) == -1) {
        perror("serverA: fail to create socket");
    }
    // bind
    if (bind(sockfd_UDP, serverAInfo->ai_addr, serverAInfo->ai_addrlen) == -1) {
        close(sockfd_UDP);
        perror("serverA: binding failed");
    }
    cout << "The server A is up and running using UDP on port <30718>" << endl;
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
    printf("serverA: waiting for servermain to startup ...\n");//
    addr_len = sizeof their_addr;
    if ((numbytes = recvfrom(sockfd_UDP, buf, MAXBUFLEN-1 , 0, 
            (struct sockaddr *)&their_addr, &addr_len)) == -1)
    {
        perror("recvfrom");
        exit(1);
    }
    
    // print received info
    printf("serverA: got packet from %s port %d\n",
    inet_ntop(their_addr.ss_family,
    get_in_addr((struct sockaddr *)&their_addr), s, sizeof s), 
    ntohs(get_in_port((struct sockaddr *)&their_addr)));
    printf("serverA: packet is %d bytes long\n", numbytes);//
    buf[numbytes] = '\0';
    printf("serverA: packet contains \"%s\"\n", buf);//

    // send msg
    string msg;
    for(int j = 0; j < countryList.size(); j++){
        msg = msg + " " + countryList[j];
    }
    if ((numbytes = sendto(sockfd_UDP, msg.c_str(), MAXBUFLEN, 0,
        serverMainInfo->ai_addr, serverMainInfo->ai_addrlen)) == -1) {
        perror("serverA: sendto");
        exit(1);
    }
    printf("serverA: sent %d bytes to %s\n", numbytes, UDP_PORT_MAIN);//

    cout << "The server A has sent a country list to Main Server" << endl;
    
}

// return three kinds of result, each result indicates the meaning as below
// 1. USER_NOT_FOUND: userID is not exist in the country
// 2. NONE: the query cannot find anyone to recommend
// 3. recommended userID: the query find a recommendation
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
    cout << "The server A is searching possible friends for User<" << userId << "> ..." << endl;
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

// listen to servermain's query request and respond with result
void listen_and_respond(){

    int numbytes;
    char buf[MAXBUFLEN];
    struct sockaddr_storage their_addr;
    socklen_t addr_len;
    char s[INET6_ADDRSTRLEN];

    // receiving
    printf("serverA: waiting for servermain requests ...\n");//
    addr_len = sizeof their_addr;
    if ((numbytes = recvfrom(sockfd_UDP, buf, MAXBUFLEN-1 , 0, 
            (struct sockaddr *)&their_addr, &addr_len)) == -1)
    {
        perror("recvfrom");
        exit(1);
    }
    
    // print received info
    printf("serverA: got packet from %s port %d\n",
    inet_ntop(their_addr.ss_family,
    get_in_addr((struct sockaddr *)&their_addr), s, sizeof s), 
    ntohs(get_in_port((struct sockaddr *)&their_addr)));
    printf("serverA: packet is %d bytes long\n", numbytes);//
    buf[numbytes] = '\0';
    printf("serverA: packet contains \"%s\"\n", buf);//

    // get request infomation
    stringstream ss(buf);
    string id;
    string country;
    ss >> id;
    ss >> country;
    cout << "The server A has received request for finding possible friends of User<" << id << "> in <" << country << ">" << endl;
    
    // process query and get the result
    string result;
    result = query(id, country);

    // send result
    if ((numbytes = sendto(sockfd_UDP, result.c_str(), result.length(), 0,
        serverMainInfo->ai_addr, serverMainInfo->ai_addrlen)) == -1) {
        perror("serverA: fail to send to main with results");
        exit(1);
    }
    printf("serverA: sent %d bytes to %s\n", numbytes, UDP_PORT_MAIN);//

    if(result == "USER_NOT_FOUND"){
        cout << "The server A has sent \"User<" << id << "> not found\" to Main Server" << endl;
    }else{
        cout << "The server A has sent the result to Main Server" << endl;
    }
}



int main(int argc, char *argv[]) 
{
    start_serverA();
    read_file();
    
    send_countrylist();

<<<<<<< HEAD
    // while(1){
    //     listen_and_respond();
    // }
=======
    while(1){
        listen_and_respond();
    }
>>>>>>> 846ccf63919bc440b9a0b1e9200a627b21cf7872


    // close socket
    freeaddrinfo(serverMainInfo);
    freeaddrinfo(serverAInfo);
    close(sockfd_UDP);
    exit(0);
<<<<<<< HEAD
}



    // string query_result = query("userID", "countryName");

    // test query
    // string q2 = query("78", "Canada");
    // string q2 = query("90", "A");
    // string q2 = query("162118937", "hSUMJxvw");
    // if(q2 == "NONE"){
    //     cout << "Here are the result: None" << endl;
    // }else if(q2 != "USER_NOT_FOUND"){
    //     cout << "Here are the result: User<";
    //     cout << q2 << ">" << endl;
    // }
    // string q3 = query("11", "Canada");
    // string q3 = query("468761846", "cYLEUu");
    // if(q3 == "NONE"){
    //     cout << "Here are the result: None" << endl;
    // }else if(q3 != "USER_NOT_FOUND"){
    //     cout << "Here are the result: User<";
    //     cout << q3 << ">" << endl;
    // }

    // pass the return value of query back to main
    
=======
}
>>>>>>> 846ccf63919bc440b9a0b1e9200a627b21cf7872

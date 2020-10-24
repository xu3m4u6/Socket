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

//UDP: 30718

vector<vector<vector<int> > > graphs; //keep the connection graph of every country
vector<string> countryList; //contains all of the country names in file
map<string,int> countryIndex; //keep country->index mapping for each country
vector<map<string, int> > userId_to_reindex; //keep userID->reindex mapping for every country
vector<map<int, string> > reindex_to_userId; //keep reindex->userID mapping for every country


// create the graph of one country
void create_graph(vector< vector<string> > allusers){
    // cout << "graph creating..." << endl;
    int index = 0;
    vector<vector<int> > adjacent_matrix(allusers.size(), vector<int> (allusers.size(), 0));
    map<string, int> user_reindex; //map userId to reindex index
    map<int, string> user_original_id; //map reindex index to userId

    // keep record of the relationship between original uerId and reindex index
    for(int i = 0; i < adjacent_matrix.size(); i++){
        string userId = allusers[i][0];
        // cout << userId << endl;
        user_reindex[userId] = index;
        user_original_id[index] = userId;
        index++;
	}
    userId_to_reindex.push_back(user_reindex);
    reindex_to_userId.push_back(user_original_id);

    // update all the connections to the adjacent matrix from allusers
    for(int i = 0; i < allusers.size(); i++){
        int currUser = user_reindex.find(allusers[i][0])->second;
        // cout << "user: " << currUser << endl;
        for(int j = 0; j < allusers[i].size(); j++){
            if(j != 0){
                int friendId = user_reindex.find(allusers[i][j])->second;
                // cout << "friend: " << friendId << endl;
                adjacent_matrix[currUser][friendId] = 1;
                // cout << "update? " << adjacent_matrix[currUser][friendId] <<endl;
                adjacent_matrix[friendId][currUser] = 1;
            }
            // cout << adjacent_matrix[i][j] << " ";
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

//construct a list of “graphs” 
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
            
            vector<string> oneUser;
            while(getline(ss,friendId,' '))
            {
                // cout << friendId << "/";
                oneUser.push_back(friendId);
            }
            // cout << endl;
            users.push_back(oneUser);
        }
	}
    create_graph(users);

    // print the list of the country
    // for(int j = 0; j < countryList.size(); j++){
    //     cout << countryList[j] << " ";
    // }
    // cout << endl;

    // print countryCount
    // cout << "total country: " << countryCount << endl;	

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
    // for (itr2 = reindex_to_userId[0].begin(); itr2 != reindex_to_userId[0].end(); ++itr2){
    //     cout << itr2->first << " => index " << itr2->second << endl;
    // }


}

void start_server(){
    cout << "The server A is up and running using UDP on port <30718>" << endl;
    
    cout << "The server A has sent a country list to Main Server" << endl;
}

int query(string userId, string countryName){

    // get the index of the country in countryList
    int countryIndex = -1;
    for(int i = 0; i < countryList.size(); i++){
        if(countryList[i] == countryName){
            countryIndex = i;
            break;
        }
    }
    cout << "country index is " << countryIndex << endl;//
    
    // check if user exist in the country
    int userReindex = -1;
    if(userId_to_reindex[countryIndex].count(userId) > 0){
        userReindex = userId_to_reindex[countryIndex].at(userId);
        cout << "user " << userId << " exist, reindex: " << userReindex << endl;//
    }else{
        cout << "User<" << userId << "> does not show up in <" << countryName << ">" << endl;
        return -1;
    }

    // recommend
    cout << "The server A is searching possible friends for User<" << userId << "> ..." << endl;
    int usersCount = userId_to_reindex[countryIndex].size(); //the number of the users in the country
    map<int, int> unconnected; //key: unconnected User's reindex -> value: number of common friends

    // check if user is the only user
    if(usersCount == 1){ return 0; }
    
    // check if target user is connected to all other user, if not,
    // find unconnected user's reindex and the number of common friends
    int target_user_friend_count = 0;
    for(int i = 0; i < graphs[countryIndex][userReindex].size(); i++){
        if(graphs[countryIndex][userReindex][i] == 1){
            target_user_friend_count++;
        }else{
            if(userReindex != i){
                // count common friends
                int commonFriendCount = 0;
                for(int j = 0; j < usersCount; j++){
                    if(graphs[countryIndex][userReindex][j] == 1 && graphs[countryIndex][i][j] == 1 ){
                        commonFriendCount++;
                    }
                }
                unconnected[i] = commonFriendCount;
            }
        }
    }

    //target user is connected to all the other users
    if(target_user_friend_count == usersCount-1){ return 0; }

    // find the max common friend count, deal with same situation
    map<int, int>::const_iterator max_itr;
    int max_common_friend_count = 0;
    int max_common_userIndex = -1; //index of the user who has max common friend with target
    for (max_itr = unconnected.begin(); max_itr != unconnected.end(); ++max_itr){
        cout << "unconnected User index: " << max_itr->first;
        cout << " => common friend count " << max_itr->second << endl;
        // if user has more common friend than current max
        if(max_itr->second > max_common_friend_count){
            max_common_friend_count = max_itr->second;
            max_common_userIndex = max_itr->first;
            cout << "max count= " << max_itr->second;
            cout << " with index= " << max_itr->first << endl;
        }else if(max_itr->second == max_common_friend_count && max_itr->second != 0){

            int curr_smallest_id = atoi(reindex_to_userId[countryIndex].at(max_common_userIndex).c_str());
            int comparing_id = atoi(reindex_to_userId[countryIndex].at(max_itr->second).c_str());
            cout << "curr_smallest_id:" << curr_smallest_id;//
            cout << " comparing_id:" << comparing_id << endl;
            if(comparing_id < curr_smallest_id){
                max_common_userIndex = max_itr->second;
            }
        }
    }
    // reindex_to_userId[countryIndex]
    
    // cout << "print out unconnected user and common friend" << endl;
    // map<int,int>::const_iterator itr;
    // for (itr = unconnected.begin(); itr != unconnected.end(); ++itr){
    //     cout << "unconnected User index: " << itr->first << " => count " << itr->second << endl;
    // }

    

    return 0; //need to fix
    

    // cout << "Here are the result: User<?id?>" << endl;
    // cout << "The server A has sent the result to Main Server" << endl;

}

void listen_to_main(){
    cout << "The server A has received request for finding possible friends of User<?id?> in <?Countryname?>" << endl;

    // int query_result = query("23", "A");
    // if(query_result == -1){
    //     cout << "The server A has sent \"User<?id?> not found\" to Main Server" << endl;
    // }

}



int main(int argc, char *argv[]) 
{
    
    read_file();
    // start_server();
    // listen_to_main();

    // test query
    // int query_result = query("90", "A");
    // int query_result2 = query("99", "A");//not exist
    // int q1 = query("3", "xYz");
    int q2 = query("78", "Canada");
    int q3 = query("11", "Canada");

    // respond_to_main();?
    
    
}

// int numCities = atoi( friendId.c_str() );
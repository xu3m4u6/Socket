# Socket

USC EE450 Socket Programming Project

a. Full Name: Yu-Yi Li

b. Student ID: 2307067718

c. Implementation

In this assignment, I created a friend recommendation socket program. It consist of a client, a servermain, and two backend servers. 
By running this program, the client can get the result of friend recommendation. The client inputs the query information and sends 
requests to servermain through TCP. When servermain received the query request, it will send the query request to backend server A 
or server B through UDP. At the backend server, they will recommend an user based on the recommendation algorithm I implemented, and
send the result back to servermain through UDP. At the end, the servermain receives the message from backend servers, and sends the
result back to client through TCP. 

d. Code files:

    1. client.cpp
        - Create TCP socket
        - Read user's query request from input
        - Send query request to servermain through TCP
        - Get recommendation result from servermain through TCP

    2. servermain.cpp
        - Create TCP and UDP sockets
        - Receive query request from client through TCP
        - Check if the query country exist
        - Decide which backend server to contact with
        - Send query request to server A or server B through UDP
        - Receive recommendation result from server A and server B through UDP
        - Send recommendation result to clients through TCP

    3. serverA.cpp
        - Create UDP socket
        - Read in data1.txt
        - Transform the data into connection graphs for each country
        - Send country list to servermain through UDP
        - Receive query request from servermain through UDP
        - Run the recommandation algorithm to get the result
        - Send recommendation result to servermain through UDP

    4. serverB.cpp
        - Create UDP socket
        - Read in data2.txt
        - Transform the data into connection graphs for each country
        - Send country list to servermain through UDP
        - Receive query request from servermain through UDP
        - Run the recommandation algorithm to get the result
        - Send recommendation result to servermain through UDP


e. The format of all the messages exchanged

  For all of the messages exchanged, I make them as a string.
    The example of the messages exchanged between servers and clients are as below:

    1. client:
        - Sending to servermain:
            Concatenates the userID and the country name as a string, and separate two parameters with a space
            e.g. When client inputs userID: 30, country name: xYz, the message will be "30 xYz"

    2. servermain
        - Sending to client:
            A string of result. The result can be userID, NONE, USER_NOT_FOUND, or COUNTRY_NOT_FOUND
            e.g. When the recommendation is userID 2, the client will received "2"
            
        - Sending to backend server (ask for country list):
            A string message as "waiting for country list"
        
        - Sending to backend server (query request):
            Concatenates the userID and the country name as a string, and separate two parameters with a space
            e.g. When servermain receives request with userID: 30, country name: xYz, the message will be "30 xYz"

    3. server A and server B
        - Sending to servermain (country list):
            Send the list of the countries as a string, each country is separated by a space
            e.g. There are three countries in data file: xYz, Canada, A. The message will be "xYz Canada A"
        
        - Sending to servermain (recommendation result):
            Send the recommendation result as a string. The result can be userID, NONE, or USER_NOT_FOUND
            e.g. When there is no user to recommend, the message will be "NONE"


g. Idiosyncrasy

  I ran the program with given testcases on Ubuntu VM. There was no idiosyncrasy found.


h. Reused code:

    Beej's Code: http://www.beej.us/guide/bgnet/
        - Create sockets
        - Getaddrinfo
        - Bind
        - Send
        - Receive





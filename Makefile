all: 
	g++ -o serverA serverA.cpp
	g++ -o serverB serverB.cpp
	g++ -o servermain servermain.cpp
	g++ -o client client.cpp	

serverA:
	./serverA

serverB:
	./serverB

servermain:
	./servermain

client:
	./client

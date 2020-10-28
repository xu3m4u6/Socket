all: 
	g++ -o serverA serverA.cpp
	g++ -o serverB serverB.cpp
	g++ -o mainserver servermain.cpp
	g++ -o client client.cpp	

.PHONY: serverA serverB mainserver

serverA:
	./serverA

serverB:
	./serverB

mainserver:
	./mainserver


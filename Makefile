# Tyler Yamashiro
# Makefile

hw4:	hw4_client.cpp hw4_server.cpp
	g++ hw4_client.cpp -std=c++11 -o pa4_client
	g++ hw4_server.cpp -std=c++11 -o pa4_server -lpthread
	
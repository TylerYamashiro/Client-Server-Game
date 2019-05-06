//TODO: set up the head part of the .cpp file
//TODO:May need to use pthreads
#include <iostream>
#include <cstring>
#include <stdlib.h>
#include <sys/types.h> // size_t, ssize_t
#include <sys/socket.h> // socket funcs
#include <netinet/in.h> // sockaddr_in
#include <arpa/inet.h> // htons, inet_pton
#include <unistd.h> // close
#include <string>
#include <limits>
#include <stdio.h>
using namespace std;

void sendString(int socket, string message){
  int stringSize = message.length();
  char mesArray [stringSize];
  strcpy(mesArray,message.c_str());
  long messageSize = sizeof(mesArray);
  //send size of message
  long networkInt = htonl(messageSize);
  int bytesSent = send(socket, (void* ) &networkInt,
                       sizeof(long), 0);
  if(bytesSent != sizeof(long)) {
    close(socket);
    exit(-1);
  }
  //send the victory message to the client
  bytesSent = send(socket, (void*) &mesArray, messageSize, 0);
  if(bytesSent != messageSize) exit(-1);
}

long recieveLong(int socket) {
    int bytesLeft = sizeof(long);
    long networkInt;
    char *bp = (char *) &networkInt;

    while (bytesLeft) {
        int bytesRecv = recv(socket, bp, bytesLeft, 0);
        if (bytesRecv <= 0){
          exit(-1);
          close(socket);
        }
        bytesLeft = bytesLeft - bytesRecv;
        bp = bp + bytesRecv;
    }

    return ntohl(networkInt);
}

char* recieveString(int socket) {
    long length = recieveLong(socket);
    char *buffer = new char [length];
    char * buffer_pointer = buffer;

    while(length){
        int bytesRecv = recv(socket, buffer_pointer, length, 0);
        length -= bytesRecv;
        buffer_pointer += bytesRecv;
        if(bytesRecv <= 0){
          close(socket);
          exit(-1);
        }
    }
    return buffer;
}

void sendLong(int socket, long number){
  long networkInt = htonl(number);
  int bytesSent = send(socket, (void* ) &networkInt,
                       sizeof(long), 0);
  if(bytesSent != sizeof(long)){
    close(socket);
    exit(-1);
  }
}


int main(int argc, char* argv[]) {
//-------------------structs and other variables--------------------------
  char *wL;
  char *bufferClear;
  long greater = 2;
  long less = 1;
  long equals = 3;
  char winner[] = "w";
  int tooHigh, tooLow, equal;
  long turn = 0;
  //Port number and server IP
  const char *IPAddr = argv[1];//IP address for server(cs1)
  unsigned short servPort = atoi(argv[2]);//port num

  //IP specific socket address
  struct sockaddr_in{
    unsigned short sin_family; //Address family (AF_INET)
    unsigned short sin_port; //Port (16-bits)
    struct in_addr sin_addr; //Internet address structure
    char sin_zero[8]; //not used
  };
  struct in_addr{
    unsigned long s_addr; //internet address (32-bits)
  };
//---------------------Create socket----------------------------------
  //create the TCP socket
  int client_socket;//hold socket number(file descriptor) if successful
  client_socket = socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);

  //Check if socket created succesfully
  if(client_socket < 0){
    cerr << "Error with socket" << endl;
    exit(-1);
  }

  //Convert dotted decimal address to internet
  unsigned long servIP;
  int status = inet_pton(AF_INET, IPAddr, (void*) &servIP);
  if(status <=0) exit(-1);

  //set the fields
  struct sockaddr_in servAddr;//maybe dont need and can use already made
  servAddr.sin_family = AF_INET;//server_address from #includes
  servAddr.sin_addr.s_addr = servIP;//can use INADDR_ANY
  servAddr.sin_port = htons(servPort);
//------------------------------------------------------------------------
  //connecting
  status = connect(client_socket,(struct sockaddr *) &servAddr,
                   sizeof(servAddr));
  if(status < 0){
    cerr <<"Error with connect" << endl;
    exit (-1);
  }
  cout <<"Welcome to Triple Number Guessing Game!"<< endl;
//-------Sending the username to server-------------------------------------
  //After communicating witht the server ask the user for their name
  string NameUser;
  cout <<"Please provide a username: ";
  cin >> NameUser; //TODO:Need to recieve as char array instead?
  cout << endl;

//--------send the username-------------------------------------
  sendString(client_socket,NameUser);
//------Get a guess from user and send to server----------------------------
  //get integers from user
  long guess1, guess2, guess3;
  bool guessCorrect = false;
  bool guessValid = false;
  while(!guessCorrect){
    turn++;
    cout << "Turn: " << turn << endl;
    tooHigh = 0;
    tooLow = 0;
    equal = 0;
    while(!guessValid){
    cout << "Please enter 3 integers between 1-200: ";
    cin >> guess1 >> guess2 >> guess3;
    cout << endl;
    //make sure the guess is valid
    if( 1 <= guess1 && guess1 <= 200 &&
        1 <= guess2 && guess2 <= 200 &&
        1 <= guess3 && guess3 <= 200)
      guessValid = true;
    else cout << "Guess invalid please try again." << endl;
    }
    guessValid = false;
//---------------send ints to the server----------------------------------
    for(int i = 0; i < 3; i++){
      long guess;
      if(i == 0) guess = guess1;
      else if(i == 1) guess = guess2;
      else guess = guess3;
      sendLong(client_socket,guess);
    }

//-------------get result from the server----------------------------
    tooLow = recieveLong(client_socket);
    tooHigh = recieveLong(client_socket);
    equal = recieveLong(client_socket);
    wL = recieveString(client_socket);

//----------------------print turn messages---------------------------------
    cout << "Too High: " << tooHigh <<"  "<< "Too Low: " << tooLow << "  "
         << "Equal: " << equal;
    cout << endl << endl;
    if(strcmp(wL,winner) == 0){
      guessCorrect = true;
    }
  }
//-------------------send turn number--------------------------------------
  //send the turn number
  sendLong(client_socket,turn);

//-------recieve the victory message and print-----------------------------
  char *vic1 = recieveString(client_socket);
  char *vic2 = recieveString(client_socket);
  char *victoryP1 = recieveString(client_socket);
  long turns = recieveLong(client_socket);
  char *victoryP2 = recieveString(client_socket);
  cout << vic1<< ' '<< vic2 << ' ' ;
  cout<< turns << ' ' <<victoryP2 << endl;
//-------------------recieve Leaderboard------------------------------------
  char *rank1 = recieveString(client_socket);
  long rank1Turns = recieveLong(client_socket);
  char *rank2 = recieveString(client_socket);
  long rank2Turns = recieveLong(client_socket);
  char *rank3 = recieveString(client_socket);
  long rank3Turns = recieveLong(client_socket);
  cout << "Leader board: "<< endl;
  cout << "1. " << rank1 << ' ' << rank1Turns << endl;
  if(rank2Turns !=  0){
    cout << "2. " << rank2 << ' ' << rank2Turns << endl;
  }
  if(rank3Turns != 0){
    cout << "3. " << rank3 << ' ' << rank3Turns << endl;
  }
  cout << endl;
//----------------reclaim memory------------------------------------------
  delete rank1;
  delete rank2;
  delete rank3;
  delete victoryP1;
  delete victoryP2;
  delete wL;
  close(client_socket);
  return 0;
}

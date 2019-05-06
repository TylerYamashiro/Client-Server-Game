#include <iostream>
#include <string>
#include <cstring>
#include <time.h>
#include <stdlib.h>
#include <sys/types.h> // size_t, ssize_t
#include <sys/socket.h> // socket funcs
#include <netinet/in.h> // sockaddr_in
#include <arpa/inet.h> // htons, inet_pton
#include <unistd.h> // close
#include "pthread.h"
#include <stdio.h>
using namespace std;
const int MAXLEADERBOARD = 3;
const int MAXPENDING = 5;
pthread_mutex_t mutex;

short numberEntries = 0;
struct{
  string name;// maybe figure out a better way to get the name
  long turns = 0;
} leaderBoard[MAXLEADERBOARD];
//------------Create struct for threads----------------------------------
struct ThreadArgs{
  int clientSock;
};

void clientProblem(int socket){
    pthread_detach(pthread_self());
    close(socket);
    cerr << "Error client has disconected" << endl;
}

void sendString(int socket, string message){
  int stringSize = message.length();
  char mesArray [stringSize];
  strcpy(mesArray,message.c_str());
  long messageSize = sizeof(mesArray);
  //send size of message
  long networkInt = htonl(messageSize);
  int bytesSent = send(socket, (void* ) &networkInt,
                       sizeof(long), 0);
  if(bytesSent != sizeof(long)){
    clientProblem(socket);
    pthread_exit(NULL);
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
          clientProblem(socket);
          pthread_exit(NULL);
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
          clientProblem(socket);
          pthread_exit(NULL);
        }
    }
    return buffer;
}

void sendLong(int socket, long number){
  long networkInt = htonl(number);
  int bytesSent = send(socket, (void* ) &networkInt,
                       sizeof(long), 0);
  if(bytesSent != sizeof(long)) {
    clientProblem(socket);
    pthread_exit(NULL);
  }
}


void *threadMain(void *args){
  int r1, r2, r3;; //send to remove stray
  long guess1, guess2, guess3;
  long tooHigh , tooLow , equal ;
  string winLoss;
  bool win = false;
  bool leaderIter = false;
  //Extract socket file descriptor from argument
  struct ThreadArgs *threadArgs = (struct ThreadArgs *) args;
  int clientSock = threadArgs -> clientSock;
  delete threadArgs;
  //TODO: add more data things if needed
//----set the random integers----------------------------------------------
  srand((unsigned)time(NULL));
  r1 = (rand() % 200) + 1;
  r2 = (rand() % 200) + 1;
  r3 = (rand() % 200) + 1;
  cout << "The three random numbers are "
      << r1 <<' '<< r2 << ' ' << r3 <<endl;
//------recieve the username-----------------------------------------------
char *userName = recieveString(clientSock);
//----------------Recieve the integers-------------------------------------
  while(!win){
    equal = 0;
    tooHigh = 0;
    tooLow = 0;
    for(int i = 0; i < 3; i++){
      long guess = recieveLong(clientSock);
      // guess = hostInt
      if(i == 0) guess1 = guess;
      else if(i ==1) guess2 = guess;
      else guess3 = guess;
    }
//---------------------check integers---------------------------------------
    //check guess 1
    if(guess1 < r1) tooLow++;
    else if(guess1 > r1) tooHigh++;
    else  equal++;
    //check guess 2
    if(guess2 < r2) tooLow++;
    else if(guess2 > r2) tooHigh++;
    else equal++;
    //check guess 3
    if(guess3 < r3) tooLow++;
    else if(guess3 > r3) tooHigh++;
    else equal++;
//----------------------send results back to client-------------------------
    sendLong(clientSock,tooLow);
    sendLong(clientSock,tooHigh);
    sendLong(clientSock,equal);
    if(equal == 3){
      win = true;
      winLoss = "w";
      sendString(clientSock,winLoss);
    }
    else{
      winLoss = "l";
      sendString(clientSock,winLoss);
    }
  }
  //recieve the turn number when game finishes
  long turn = recieveLong(clientSock);
//-------------send victory message------------------------------------------
  //create the string
  string vic = "Congratulations! It";
  string vic2 ="took";
  string victoryP1 = "Congratulations! It took";
  string victoryP2 = "turns to guess all three numbers!";
  sendString(clientSock,vic);
  sendString(clientSock,vic2);
  sendString(clientSock,victoryP1);
  sendLong(clientSock,turn);
  sendString(clientSock, victoryP2);

//----------------------update leaderBoard----------------------------------
  //mutual exclusion to global variables
  pthread_mutex_lock(&mutex);
  //iterate through entire leaderboard array
  for(int i = 0; i < 3; i++){
    //if the score has not found a place on the leaderboard
    if(leaderIter == false){
      //if the spot on the leaderboard is empty insert score and name
      if(leaderBoard[i].turns == 0){
        leaderBoard[i].turns = turn;
        leaderBoard[i].name = userName;
        //memset(leaderBoard[i].name, '\0',50);
        //strcpy(leaderBoard[i].name,userName);
        leaderIter = true;
        numberEntries++;
      }
      //if the number of turns are less than something on the leaderBoard
      //insert it into the appropriate spot
      else if(turn < leaderBoard[i].turns ){
        //check if number of entries is less than 3
        if(numberEntries < 3){
          leaderBoard[i+1].turns = leaderBoard[i].turns;
          leaderBoard[i+1].name = leaderBoard[i].name;
          //memset(leaderBoard[i+1].name, '\0',50);
          //strcpy(leaderBoard[i+1].name,
              //  leaderBoard[i].name);
          leaderBoard[i].turns = turn;
          leaderBoard[i].name = userName;
          //memset(leaderBoard[i].name, '\0',50);
          //strcpy(leaderBoard[i].name,userName);
          numberEntries++;
          leaderIter = true;
        }
        //if not less than 3 insert and shift and kick off last score
        else if(numberEntries >= 3){
          for(int j = 2; j - i > 0; j--){
            leaderBoard[j].turns =
            leaderBoard[j-1].turns;
            leaderBoard[j].name = leaderBoard[j-1].name;
            //memset(leaderBoard[j].name, '\0',50);
            //strcpy(leaderBoard[j].name,
                //  leaderBoard[j-1].name);
          }
          leaderBoard[i].turns = turn;
          leaderBoard[i].name = userName;
        //  memset(leaderBoard[i].name, '\0',50);
          //strcpy(leaderBoard[i].name,userName);
          leaderIter = true;
        }

      }
      //if there is a tie with a score in the leaderboard
      else if(turn == leaderBoard[i].turns){
        int lastMatch;
        //find the last instance of a tie
        for(int j = 0; j < 3; j++){
          if(turn == leaderBoard[j].turns){
            lastMatch = j;
          }
        }
        //if it ties withe the first score put it in position 2 and shift
        if(lastMatch == 0){
          leaderBoard[2].turns =
          leaderBoard[1].turns;
        /*  memset(leaderBoard[2].name, '\0',50);
          strcpy(leaderBoard[2].name,
                leaderBoard[1].name);*/
          leaderBoard[2].name = leaderBoard[1].name;
          leaderBoard[1].turns = turn;
          /*
          memset(leaderBoard[1].name, '\0',50);
          strcpy(leaderBoard[1].name,userName);
          */
          leaderBoard[1].name = userName;
          numberEntries++;
          leaderIter = true;
        }
        //if it ties with position 2 insert it into position 3
        else if(lastMatch == 1){
          leaderBoard[2].turns = turn;
          //memset(leaderBoard[2].name, '\0',50);
          //strcpy(leaderBoard[2].name,userName);
          leaderBoard[2].name = userName;
          numberEntries++;
          leaderIter = true;
        }
      }
    }
  }
//-------------------------send the leaderboard to client--------------------
  for(int i = 0; i < 3; i++){
    long turnHolder =leaderBoard[i].turns;
    string nameHolder = leaderBoard[i].name;
    //send Name
    sendString(clientSock,nameHolder);
    //send Turn
    long networkInt = htonl(turnHolder);
    int bytesSent = send(clientSock, (void* ) &networkInt,
                          sizeof(long), 0);
    if(bytesSent != sizeof(long)) exit(-1);
  }
  //unlock the section
  pthread_mutex_unlock(&mutex);
//----------------------------------------------------------------------

  delete userName;
  pthread_detach(pthread_self());
  close(clientSock);
  return NULL;
}

//==========================================================================
int main(int argc, char* argv[]){
  //init the mutex variable
  pthread_mutex_init(&mutex,NULL);
//------------Create socket-----------------------------------------------
  //create socket
  int server_socket = socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
//-----------Bind port to socket------------------------------------------
  //set the Port
  unsigned short servPort = atoi(argv[1]);
  struct sockaddr_in servAddr;
  servAddr.sin_family = AF_INET;
  servAddr.sin_addr.s_addr = htonl(INADDR_ANY);
  servAddr.sin_port = htons(servPort);
//bind the socket
  int status;
  status = bind(server_socket,(struct sockaddr *) &servAddr, sizeof(servAddr));
  if(status < 0){
    cerr << "Error with bind" << endl;
    exit(-1);
  }
//---------Set socket to listen-----------------------------------------------
  status = listen(server_socket,MAXPENDING);
  if(status < 0){
    cerr << "Error with listen" << endl;
    exit(-1);
  }
//----------------------------------------------------------------------------
  while(true){
    //Accept connection from client
    struct sockaddr_in clientAddr;
    socklen_t aLen = sizeof(clientAddr);
    int clientSock;
    clientSock = accept(server_socket, (struct sockaddr *) &clientAddr,&aLen);
    if(clientSock < 0) exit(-1);
    //create and initialize argument structs
    struct ThreadArgs *threadArgs;
    threadArgs = new struct ThreadArgs;
    threadArgs -> clientSock = clientSock;
    //create client thread
    pthread_t threadID;
    status = pthread_create(&threadID, NULL, threadMain,
                            (void*) threadArgs);
    if(status != 0) exit(-1);

  }

  return 0;
}

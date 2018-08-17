#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include <pthread/pthread.h>
#include <poll.h>

#define PORT 4000
#define QLEN 100
#define BLEN 512
#define MAX_SOCKS 512
#define POLL_TIMEOUT -1

void* handleClient(void* args);
void* dispatchMessageToAllSocks(void* args);

int sendToAllPipeFds[2];
struct pollfd fdToPoll;

int main(int argc, char* argv[]) {
  int allClientDescriptorsIdx = 0;
  int allClientDescriptors[MAX_SOCKS] = {-1};
  struct sockaddr_in serverAddrInfo, clientAddrInfo;
  int serverDescriptor, clientDescriptor;

  serverDescriptor = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
  
  memset((char*)&serverAddrInfo, 0, sizeof(struct sockaddr_in));
  serverAddrInfo.sin_family = AF_INET;
  serverAddrInfo.sin_addr.s_addr = INADDR_ANY;
  serverAddrInfo.sin_port = htons((u_short) PORT);

  bind(serverDescriptor, (struct sockaddr *)&serverAddrInfo, sizeof(struct sockaddr_in));

  // Create send to all thread
  pipe(sendToAllPipeFds);
  fdToPoll.fd = sendToAllPipeFds[0];
  fdToPoll.events = POLLIN;
  
  pthread_t sendToAllThread;
  pthread_create(&sendToAllThread, NULL, dispatchMessageToAllSocks, (void*)allClientDescriptors);
  
  // TODO TESTING
  send(sendToAllPipeFds[1], "TEST MESSAGE TO SENDER THREAD", strlen("TEST MESSAGE TO SENDER THREAD"), 0);
  

  listen(serverDescriptor, QLEN);
  printf("Listening ...\n");
  while (1) {
    int len = sizeof(struct sockaddr_in);
    clientDescriptor = accept(serverDescriptor, (struct sockaddr*) &clientAddrInfo, (socklen_t *)&len);
    if (clientDescriptor) {
      allClientDescriptors[allClientDescriptorsIdx] = clientDescriptor;
      allClientDescriptorsIdx++;
      printf("Got new connection\n");
      // TODO how to join these threads
      pthread_t clientThread;
      pthread_create(&clientThread, NULL, handleClient, (void*)&clientDescriptor);
    }
  }
  pthread_join(sendToAllThread, NULL);
  return 0;
}

void* handleClient(void* args) {
  int clientDescriptor = *((int *)args);
  printf("Listening to client on descriptor %i\n", clientDescriptor);
  char buf[BLEN] = {0}; 
  while(1) {
    if (recv(clientDescriptor, buf, BLEN, 0) > 0) {
      printf("Recevied: %s\n", buf);
      write(sendToAllPipeFds[1], buf, strlen(buf));
      memset(buf, 0, BLEN);
    }
  }
}

void* dispatchMessageToAllSocks(void* args) {
  printf("Started dispatching thread for all sockets\n");
  int* allSockDesciptors = (int*)args;
  char buf[BLEN] = {0};
  while(1) {
    poll(&fdToPoll, 1, POLL_TIMEOUT);
    if (fdToPoll.revents && POLLIN) {
      size_t numBytesRead = read(fdToPoll.fd, buf, BLEN);
      if (numBytesRead) {
        int i = 0;
        while(i < MAX_SOCKS && allSockDesciptors[i] != -1) {
          send(allSockDesciptors[i], buf, strlen(buf), 0);
          i++;
        }
        memset(buf, 0, BLEN);
      }
    }
  }
}

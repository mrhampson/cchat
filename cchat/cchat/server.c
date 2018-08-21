#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include <pthread.h>
#include <poll.h>
#include <stdbool.h>

#define PORT 4000
#define QLEN 100
#define BLEN 1024
#define MAX_SOCKS 512
#define POLL_TIMEOUT -1
#define USER_NAME_MAX 128

struct client_thread_info {
  int descriptor;
  struct sockaddr_in clientAddrInfo;
  char name[USER_NAME_MAX];
};

void* handleClient(void* args);
void* dispatchMessageToAllSocks(void* args);
bool startsWith(const char *pre, const char *str);
void removeLastChar(char* str);

// Shared across threads
int sendToAllPipeFds[2];
struct pollfd fdToPoll;
int allClientDescriptorsIdx = 0;
int allClientDescriptors[MAX_SOCKS] = {-1};

int main(int argc, char* argv[]) {
  struct sockaddr_in serverAddrInfo, clientAddrInfo;
  struct client_thread_info clientInfo;
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

  listen(serverDescriptor, QLEN);
  printf("Listening ...\n");
  while (1) {
    int len = sizeof(struct sockaddr_in);
    clientDescriptor = accept(
      serverDescriptor,
      (struct sockaddr*)&clientAddrInfo,
      (socklen_t *)&len);
    if (clientDescriptor) {
      allClientDescriptors[allClientDescriptorsIdx] = clientDescriptor;
      allClientDescriptorsIdx++;
      clientInfo.descriptor = clientDescriptor;
      clientInfo.clientAddrInfo = clientAddrInfo;
      inet_ntop(AF_INET, &(clientInfo.clientAddrInfo.sin_addr), clientInfo.name, INET_ADDRSTRLEN);
      printf("Got new connection\n");
      // TODO how to join these threads
      pthread_t clientThread;
      pthread_create(&clientThread, NULL, handleClient, (void*)&clientInfo);
    }
  }
  return 0;
}

void* handleClient(void* args) {
  struct client_thread_info clientThreadInfo = *((struct client_thread_info *)args);
  printf("Listening to client %s on descriptor %i\n", clientThreadInfo.name, clientThreadInfo.descriptor);
  char buf[BLEN] = {0}; 
  while(1) {
    if (recv(clientThreadInfo.descriptor, buf, BLEN, 0) > 0) {
      removeLastChar(buf); //removes the new line
      printf("Recevied from (%s): %s\n", clientThreadInfo.name, buf);
      if (startsWith("NICK ", buf)) {
        strncpy(clientThreadInfo.name, (buf + 5 * sizeof(char)), USER_NAME_MAX);
      }
      char formattedResponse[BLEN];
      snprintf(formattedResponse, BLEN, "%s: %s\n", clientThreadInfo.name, buf);
      write(sendToAllPipeFds[1], formattedResponse, strlen(formattedResponse));
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

// From https://stackoverflow.com/questions/4770985/how-to-check-if-a-string-starts-with-another-string-in-c
bool startsWith(const char *pre, const char *str) {
  size_t lenpre = strlen(pre),
  lenstr = strlen(str);
  return lenstr < lenpre ? false : strncmp(pre, str, lenpre) == 0;
}

void removeLastChar(char* str) {
  unsigned long len = strlen(str);
  str[len-1] = 0;
}

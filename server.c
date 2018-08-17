#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>

#define PORT 4897
#define QLEN 100
#define BLEN 10

int main(int argc, char *argv[]) {
  struct sockaddr_in serverAddrInfo, clientAddrInfo;
  int serverDescriptor, clientDescriptor;
  char buf[BLEN];

  serverDescriptor = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
  
  memset((char *) &serverAddrInfo, 0, sizeof(struct sockaddr_in));
  serverAddrInfo.sin_family = AF_INET;
  serverAddrInfo.sin_addr.s_addr = INADDR_ANY;
  serverAddrInfo.sin_port = htons((u_short) PORT);

  bind(serverDescriptor, (struct sockaddr *)&serverAddrInfo, sizeof(struct sockaddr_in));

  listen(serverDescriptor, QLEN);

  while (1) {
    int len = sizeof(struct sockaddr_in);
    clientDescriptor = accept(serverDescriptor, (struct sockaddr *) &clientAddrInfo, (socklen_t *)&len);
    memset(buf, 0, BLEN);
    recv(clientDescriptor, buf, BLEN, 0);
    send(clientDescriptor, buf, strlen(buf), 0);
  }
  return 0;
}

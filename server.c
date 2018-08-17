#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>

#define net_assert(err, errmsg) { if ((err)) { perror(errmsg); assert(!(err)); } }

#define PORT 4897
#define QLEN 100
#define BLEN 256

int visits;

int
main(int argc, char *argv[])
{
  struct sockaddr_in serverAddrInfo, clientAddrInfo;
  struct hostent *clientHostInfo;
  int serverDescriptor, clientDescriptor;
  int len;
  char buf[BLEN];

  serverDescriptor = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
  
  memset((char *) &serverAddrInfo, 0, sizeof(struct sockaddr_in));
  serverAddrInfo.sin_family = AF_INET;
  serverAddrInfo.sin_addr.s_addr = INADDR_ANY;
  serverAddrInfo.sin_port = htons((u_short) PORT);

  bind(serverDescriptor, (struct sockaddr *) &serverAddrInfo, sizeof(struct sockaddr_in));

  listen(serverDescriptor, QLEN);

  while (1) {
    len = sizeof(struct sockaddr_in);
    clientDescriptor = accept(serverDescriptor, (struct sockaddr *) &clientAddrInfo, (socklen_t *)&len);

    clientHostInfo = gethostbyaddr((char *) &clientAddrInfo.sin_addr, sizeof(struct in_addr), AF_INET);
    printf("Connected from %s\n", 
          ((clientHostInfo && clientHostInfo->h_name) ? clientHostInfo->h_name : inet_ntoa(clientAddrInfo.sin_addr)));

    visits++;
    sprintf(buf, "This server has been contacted %d time(s).\n", visits);
    send(clientDescriptor, buf, strlen(buf), 0);
    close(clientDescriptor);
  }
  exit(0);
}

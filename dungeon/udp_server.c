#include "interfaces.h"
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define SERVER_PORT 3555
#define BUFFER_LEN 100
#define FALSE 0

int main(void) {
  int sd = -1, rc = 0;
  char buffer[BUFFER_LEN];
  struct sockaddr_in serveraddr;
  struct sockaddr_in clientaddr;
  socklen_t clientaddrlen = sizeof(clientaddr);
  do {

    sd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sd < 0) {
      perror("socket() failed");
      break;
    }

    memset(&serveraddr, 0, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_port = htons(SERVER_PORT);
    // memcpy(&serveraddr.sin_addr.s_addr, ip, strlen(ip));
    //  memcpy(&serveraddr.sin6_addr, &in6addr_any, sizeof(in6addr_any));

    struct ifconf *ifc = (struct ifconf *)malloc(sizeof(struct ifconf));
    int interfaces = 1;
    struct ipaddr_dummy *addr_list = interfacesToIpV4addr(sd, ifc, &interfaces);
    if (!addr_list) {
      exit(1);
    }
    // int s = inet_pton(AF_INET, ip, &serveraddr.sin_addr.s_addr);
    // if (s < 0) {
    //   if (s == 0)
    //     fprintf(stderr, "not in presentation format\n");
    //   perror("inet_pton()");
    // }
    printf("%s\n", addr_list[1].data);

    serveraddr.sin_addr.s_addr = inet_addr((addr_list + 1)->data);

    rc = bind(sd, (struct sockaddr *)&serveraddr, sizeof(serveraddr));

    if (rc < 0) {
      perror("bind() failed");
      break;
    }

    rc = recvfrom(sd, buffer, sizeof(buffer), 0, (struct sockaddr *)&clientaddr,
                  &clientaddrlen);

    if (rc < 0) {
      perror("recvfrom() failed");
      break;
    }
    printf("server received the following: <%s>\n", buffer);
    inet_ntop(AF_INET, &clientaddr.sin_addr.s_addr, buffer, sizeof(buffer));
    printf("from port %d and address %s\n", ntohs(clientaddr.sin_port), buffer);

    rc = sendto(sd, buffer, sizeof(buffer), 0, (struct sockaddr *)&clientaddr,
                sizeof(clientaddr));

    if (rc < 0) {
      perror("sendto() failed");
      break;
    }
    free(ifc);

  } while (FALSE);

  if (sd != 1)
    close(sd);
}

#ifndef CONNECTION_H
#define CONNECTION_H
#include <arpa/inet.h>
#include <cstring>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
/* Obtain address(es) matching host/port. */

void obtainAddresses(char *ip_addr, char *port, struct addrinfo *hints,
                     struct addrinfo **result) {

  int s = getaddrinfo(ip_addr, port, hints, result);
  if (s != 0) {
    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
    exit(EXIT_FAILURE);
  }
}
void connect2Addr(struct addrinfo *result, int *sfd
                  ) {
  /* getaddrinfo() returns a list of address structures.
     Try each address until we successfully connect(2).
     If socket(2) (or connect(2)) fails, we (close the socket
     and) try the next address. */

  struct addrinfo *rp;
  for (rp = result; rp != NULL; rp = rp->ai_next) {
    *sfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
    if (*sfd == -1)
      continue;
    break;
    close(*sfd);
  }
  freeaddrinfo(result); /* No longer needed */
  if (rp == NULL) {     /* No address succeeded */
    fprintf(stderr, "Could not connect\n");
    exit(EXIT_FAILURE);
  }
}
// here we are binding the socket to the first address that is able to
// but we want to modify this to bind to all possible addresses
// maybe with an exception for localhost
// but what happens if you bind a socket to more than one address?
void bindAddress(struct addrinfo *result, int *sfd) {
  /* getaddrinfo() returns a list of address structures.
     Try each address until we successfully connect(2).
     If socket(2) (or connect(2)) fails, we (close the socket
     and) try the next address. */

  struct addrinfo *rp;
  for (rp = result; rp != NULL; rp = rp->ai_next) {
    /* *sfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
    if (*sfd == -1)
      continue; */
    /* if (setsockopt(*sfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0){
      perror("setsockopt() failed");
    } */
    if (bind(*sfd, rp->ai_addr, rp->ai_addrlen) != -1){
      fprintf(stdout, "Bind successful\n");
      char addr[16];
      inet_ntop(rp->ai_family, rp->ai_addr->sa_data + 2, addr, sizeof(addr));
      printf("%s\n", addr);
      break; /* Success */
    }

    close(*sfd);
  }
  // freeaddrinfo(result); /* No longer needed */
  if (rp == NULL) { /* No address succeeded */
    perror("bind()");
    fprintf(stderr, "Could not connect\n");
    exit(EXIT_FAILURE);
  }
}
#endif

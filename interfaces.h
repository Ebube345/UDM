#ifndef INTERFACES_H
#define INTERFACES_H

#include "asio.hpp"

#define ADDR_LEN 15
struct ipaddr_dummy {
  sa_family_t family;
  char data[ADDR_LEN];
  struct ipaddr_dummy *next;
  bool valid =
      false; // valid means that the interface is 1. up and 2. can be
             // reachable by the client at the other end of the connection
};
struct ipaddr_list {
  int len;
  struct ipaddr_dummy *head;
};
struct ipaddr_dummy *interfacesToIpV4addr(int s, struct ifconf *ifc,
                                          int *numreqs);
int _noOfInterfaces(int s, struct ifconf *ifc);
struct ipaddr_dummy *interfacesToIpV4addr(int s, struct ifconf *ifc,
                                          int *numreqs);
#endif
void makeSocketNifc(int *s);

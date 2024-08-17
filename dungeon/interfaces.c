#include "interfaces.h"
void makeSocketNifc(int *s) {
  struct sockaddr_in addr;
  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  int adt = INADDR_ANY;
  memcpy(&addr.sin_addr, &adt, sizeof(INADDR_ANY));
  *s = socket(AF_INET, SOCK_DGRAM, 0);
  if (*s < 0) {
    perror("Unable to create socket");
  }
  // int interfaces = 1;
  //  struct ipaddr_dummy *addr_list = interfacesToIpV4addr(s, &ifc,
  //  &interfaces); for (int i = 0; i < interfaces; i++) {
  //    printf("%s\n", addr_list->data);
  //    addr_list++;
  //  }
}

int _noOfInterfaces(int s, struct ifconf *ifc) {
  int numreqs = 1;
  for (;;) {
    ifc->ifc_len = sizeof(struct ifreq) * numreqs;
    ifc->ifc_buf = realloc(ifc->ifc_buf, ifc->ifc_len);
    if (ioctl(s, SIOCGIFCONF, ifc) < 0) {
      perror("SIOCGIFCONF");
      return numreqs;
    }
    if (ifc->ifc_len == (int)sizeof(struct ifreq) * numreqs) {
      numreqs *= 2;
      continue;
    }
    break;
  }
  return ifc->ifc_len / sizeof(struct ifreq);
}
struct ipaddr_dummy *interfacesToIpV4addr(int s, struct ifconf *ifc,
                                          int *numreqs) {
  struct ipaddr_dummy *list;
  int interfaces = _noOfInterfaces(s, ifc);
  *numreqs = interfaces;
  list =
      (struct ipaddr_dummy *)malloc(sizeof(struct ipaddr_dummy) * interfaces);
  struct ifreq *ifr = ifc->ifc_req;
  char buffer[32];
  for (int n = 0, i = 0; n < ifc->ifc_len; n += sizeof(struct ifreq), i++) {
    inet_ntop(AF_INET, ifr->ifr_addr.sa_data + 2, buffer, sizeof(buffer));
    strcpy((list + i)->data, buffer);
    ifr++;
  }
  return list;
}

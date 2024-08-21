#ifndef GATEWAY_H
#define GATEWAY_H

#include "connection.h"
#include "datafactory.h"
#include "interfaces.h"
#include "stdutils.h"
#include <cassert>
#include <cstdlib>
#include <map>
#include "worker_pool.hpp"
namespace Pipeline {
// gets the inetface that's currently being used
// gets the other valid interfaces
// makes available those interfaces
// return the current state of each interface - if it is usable for a new
// subconnection
class PhyInterfaces {
#define ADDR_LEN 15
  struct ifconf *ifc;
  int infc;
  int &sfd;
  uint16_t indx;
  struct addrinfo *addresses;
  struct ipaddr_list {
    int len;
    struct ipaddr_dummy *head;
  };

  int _noOfInterfaces() {
    int numreqs = 1;
    for (;;) {
      ifc->ifc_len = sizeof(struct ifreq) * numreqs;
      ifc->ifc_buf = (char *)realloc(ifc->ifc_buf, ifc->ifc_len);
      if (ioctl(sfd, SIOCGIFCONF, ifc) < 0) {
        perror("SIOCGIFCONF");
        return numreqs;
      }
      if (ifc->ifc_len == (int)sizeof(struct ifreq) * numreqs) {
        numreqs *= 2;
        continue;
      }
      break;
    }
    int addresses =  ifc->ifc_len / sizeof(struct ifreq);
    return addresses;
  }

  struct ipaddr_dummy *interfacesToIpV4addr(struct ifconf *ifc,
                                            int *numreqs) {
    struct ipaddr_dummy *list;
    int interfaces = _noOfInterfaces();
    *numreqs = interfaces;
    list =
        (struct ipaddr_dummy *)malloc(sizeof(struct ipaddr_dummy) * interfaces);
    struct ifreq *ifr = ifc->ifc_req;
    char buffer[32];
    for (int n = 0, i = 0; n < ifc->ifc_len; n += sizeof(struct ifreq), i++) {
      inet_ntop(AF_INET, ifr->ifr_addr.sa_data + 2, buffer, sizeof(buffer));
      list->family = ifr->ifr_addr.sa_family;
      (list + i)->family = ifr->ifr_addr.sa_family;
      strcpy((list + i)->data, buffer);
      ifr++;
    }
    return list;
  }

public:
  struct ipaddr_dummy *addressList = NULL;
  struct ipaddr_dummy *peer_addressList = NULL;
  int peer_addressCount = 0;
  PhyInterfaces(int &_sfd, struct addrinfo *addr) : sfd(_sfd), addresses(addr) {
    ifc = (struct ifconf *)malloc(sizeof(struct ifconf));
    infc = _noOfInterfaces();
    addressList = interfacesToIpV4addr(ifc, &infc);
    if (!addressList) {
      //fprintf(stderr, "Unable to find any interface\n");
      exit(1);
    }
    for (int i = 0; i < infc; i++) {
      _LOG(addressList[i].data);
    }
  }
  ~PhyInterfaces() {
    free(ifc);
    free(addressList);
    free(peer_addressList);
  }
  /*
   * Returns a linked list of all the interfaces
   * */
  struct ipaddr_dummy *getInterfaces() { return addressList; };
  struct ipaddr_dummy *getInterface() { return &addressList[indx++]; };
  bool isValid(struct ipaddr_dummy *addr) { return addr->valid; };
  void getInfAddr();
  int getInfCount() { return _noOfInterfaces(); };
  void setInfCount(int count) { infc = count; };
};
class Timer {};
class GatewayProcessor {
public:
  void process(struct memBuff *buf, stateCodes) {
    if (buf->data->header.capType & (uint16_t)capsuleType::PAYLD) {
    }
  }
  void setCapParams(capsule *buf, stateCodes){
    if (buf) {
      memcpy(buf->body, &buf->header, HEADER_SPACE);
      //fprintf(stdout, "sending buffer at index [%u]\n", buf->header.capid);
    }
  }
  void stripHeader(capsule *buf) {
    buf->header = *(capsuleHdr *)buf->body;
    // adjust buffer body after removing header
    buf->body = buf->body + sizeof(capsuleHdr);
  }
};
class Gateway {
public:
  Gateway(int &sfd, struct sockaddr_storage &peer_addr,
          __socklen_t &peer_addrlen, struct addrinfo *addr,
          struct DataFactory *txFactory, struct DataFactory *rxFactory)
      : sfd(sfd), peer_addr(peer_addr), peer_addrlen(peer_addrlen),
        txFactory(txFactory), rxFactory(rxFactory), interfaces(sfd, addr) {
    processor = std::unique_ptr<GatewayProcessor>(new GatewayProcessor());
  }
  ~Gateway() {}
  void chunkWrite(struct capsule *chunk2Write) {
    memcpy(chunk2Write->body, &chunk2Write->header, HEADER_SPACE);
    // The connection would be a guide to selecting an interface
    struct sockaddr *s = selectAddrFromList();
    char addr[16];
    inet_ntop(AF_INET, s->sa_data + 2, addr, sizeof(addr));
    //fprintf(stdout, "sending capsule on address: %s and port %d\n", addr, ntohs(((struct sockaddr_in *)s)->sin_port));
    long int amt2Send = chunk2Write->header.capSize + sizeof(capsuleHdr);
    if (sendto(sfd, chunk2Write->body, amt2Send, 0, s,
               sizeof(peer_addr)) != amt2Send) {
      //fprintf(stderr, "no/partial write\n");
      perror("sendto");
    } else {
      //fprintf(stderr, "Wrote %lu bytes to peer\n", amt2Send);
    }
  }
  void send(capsule *chunk2Send) {
    static int counter = 0;
    static int ttlsent = 0;
    ssize_t nread;
    /// set the capsule size in case it has been updated by another stage
    auto amt2Send = chunk2Send->header.capSize + sizeof(capsuleHdr);
    // copy header to the buffer body
    struct sockaddr *s = selectAddrFromList();
    char addr[16];
    inet_ntop(AF_INET, s->sa_data + 2, addr, sizeof(addr));
    //short portno = ((struct sockaddr_in *)s)->sin_port;
    _LOG(addr);
    memcpy(chunk2Send->body, &chunk2Send->header, HEADER_SPACE);
    if ((nread = sendto(sfd, chunk2Send->body, amt2Send, 0, s,
                        sizeof(struct sockaddr))) <= 0) {
      perror("sendto");
      exit(EXIT_FAILURE);
    }
    //fprintf(stdout, "[%d]sent %zd bytes to the receiver\n", counter, nread);
    ttlsent += nread;
    // fprintf(stdout, "ttl bytes sent = %d and current size in factory = %d\n",
    //         ttlsent, txFactory->curSize);
    //txFactory->disposeAllocedBody(chunk2Send->data.get());
    counter++;
  }
  struct memBuff *pull() {
    struct memBuff *emptyChunk = rxFactory->makeChunkforRx(rxFactory);
    //std::lock_guard<std::mutex> chunk_lock(memBuff_mtx);
    if(!emptyChunk->data) std::cin.get();
    ssize_t nread = -1;
    static int counter = 0;
    peer_addrlen = sizeof(peer_addr);
    nread = recvfrom(sfd, emptyChunk->data->body, rxFactory->chunkSize, 0,
                     (struct sockaddr *)&peer_addr, &peer_addrlen);
    if (nread == -1) {
      perror("read()");
      exit(EXIT_FAILURE);
    }
    emptyChunk->size = nread;
    processor->stripHeader(emptyChunk->data.get());
    _LOG(emptyChunk->data->header.capid);
    //fprintf(stdout, "[%d] -> read %zd bytes \n", counter, nread);
    counter++;
    return emptyChunk;
  }
  void receive(struct memBuff *chunk2recv) {
    char host[NI_MAXHOST], service[NI_MAXSERV];
    peer_addrlen = sizeof(peer_addr);
    ssize_t nread = 0;
    if ((nread = recvfrom(sfd, chunk2recv->data->body, chunk2recv->size, 0,
                          (struct sockaddr *)&peer_addr, &peer_addrlen)) ==
        -1) {

      perror("recvfrom()");
      //fprintf(stderr, "Ignoring a failed request\n");
      exit(EXIT_FAILURE);
    };
    int s = getnameinfo((struct sockaddr *)&peer_addr, peer_addrlen, host,
                        NI_MAXHOST, service, NI_MAXSERV, NI_NUMERICSERV);
    if (s == 0) {
      printf("Received %zd bytes from %s:%s\n", nread, host, service);
      chunk2recv->size = nread;
      processor->stripHeader(chunk2recv->data.get());
    } else {
      //fprintf(stderr, "getnameinfo: %s\n", gai_strerror(s));
      exit(EXIT_FAILURE);
    }
  }
  // TODO: This function should be part of a scheduler claass later on - and
  // would depend on the congestion control and connection
  struct sockaddr *selectAddrFromList() {
#define MAX_INF interfaces.peer_addressCount
    static int i = 0;
    if (interfaces.peer_addressList) {
      struct sockaddr_in *s =
          (struct sockaddr_in *)malloc(sizeof(struct sockaddr_in));
      s->sin_family = AF_INET;
      s->sin_port = ((struct sockaddr_in *)&peer_addr)->sin_port;
      s->sin_addr.s_addr =
          inet_addr(interfaces.peer_addressList[++i % MAX_INF].data);
      return (struct sockaddr *)s;
    }
    return (struct sockaddr *)&peer_addr;
  };

private:
  std::unique_ptr<GatewayProcessor> processor;
  int &sfd;
  struct sockaddr_storage &peer_addr;
  socklen_t &peer_addrlen;
  Timer gatewayTimer;
  struct DataFactory *txFactory;
  struct DataFactory *rxFactory;
public:
  PhyInterfaces interfaces;

};
} // namespace Pipeline
#endif

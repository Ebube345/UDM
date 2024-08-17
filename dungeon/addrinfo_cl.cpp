// #include "LZSS-Compression/LZSS.h"
#include "connection.h"
#include "datafactory.h"
#include "session.h"

int not_main(int argc, char *argv[]) {
  if (argc != 5) {
    fprintf(stderr, "Usage: %s host port msg fileout...\n", argv[0]);
    exit(EXIT_FAILURE);
  }
  int sfd, s;
  size_t len;
  ssize_t nread;
  struct addrinfo hints;
  struct addrinfo *srv_result,*cl_result, *rp;

  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_UNSPEC;    /* Allow IPv4 or IPv6 */
  hints.ai_socktype = SOCK_DGRAM; /* Datagram socket */
  hints.ai_flags = 0;
  hints.ai_protocol = 0; /* Any protocol */

  struct sockaddr_storage peer_addr;
  socklen_t peer_addrlen = sizeof(peer_addr);
  char port[] = "30033";
  /* Obtain address(es) matching host/port. */
  obtainAddresses(argv[1], argv[2], &hints, &srv_result);
  obtainAddresses(NULL, port, &hints, &cl_result);
  memcpy(&peer_addr , (struct sockaddr_storage *)srv_result->ai_addr, sizeof(peer_addr));
  if (cl_result) {
    bindAddress(cl_result, &sfd);
    connect2Addr(srv_result, &sfd, &peer_addr);
  } else {
    fprintf(stderr, "Could not obtain any address\n");
    exit(EXIT_FAILURE);
  }

#define ALLOC_SIZE 2048
  /* DataSource sinkFile =
      DataSource(argv[4], sourceMode::OUT); // sending out to this file
  struct DataFactory txFac = {.buffer =
                                  threadsafe_queue<struct memBuff>(ALLOC_SIZE),
                              .allocSize = ALLOC_SIZE,
                              .curSize = 0,
                              .curPos = 0,
                              .curWritePos = 0,
                              .curReadPos = 0,
                              .chunkSize = 1480,
                              .sourceData = NULL};
  struct DataFactory rxFac = {.buffer =
                                  threadsafe_queue<struct memBuff>(ALLOC_SIZE),
                              .allocSize = ALLOC_SIZE,
                              .curSize = 0,
                              .curPos = 0,
                              .curWritePos = 0,
                              .curReadPos = 0,
                              .chunkSize = 1480,
                              .sourceData = &sinkFile};
  Pipeline::gateway = std::unique_ptr<Pipeline::Gateway>(new Pipeline::Gateway(
      sfd, peer_addr, peer_addrlen, srv_result, &txFac, &rxFac));
  Pipeline::session = std::unique_ptr<Pipeline::Session>(
      new Pipeline::Session(stateDirection::RX, &txFac, &rxFac));
  Pipeline::compositor = std::unique_ptr<Pipeline::Compositor>(
      new Pipeline::Compositor(stateDirection::RX, &txFac, &rxFac));
  assert(Pipeline::compositor.get());
  assert(Pipeline::session.get());
  assert(Pipeline::gateway.get());
  Pipeline::session->start();
  Pipeline::session->destroy();
  destroyDataUtil(&txFac);
  destroyDataUtil(&rxFac); */
  exit(EXIT_SUCCESS);
}

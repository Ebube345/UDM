#include "connection.h"
#include "datafactory.h"
#include "session.h"

int not_main(int argc, char *argv[]) {
  int sfd, s;
  ssize_t nread;
  struct addrinfo hints;
  struct addrinfo *result, *rp;
  socklen_t peer_addrlen;
  struct sockaddr_storage peer_addr;
  if (argc != 3) {
    fprintf(stderr, "Usage: %s port file2Send\n", argv[0]);
    exit(EXIT_FAILURE);
  }

  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_UNSPEC;    /* Allow IPv4 or IPv6 */
  hints.ai_socktype = SOCK_DGRAM; /* Datagram socket */
  hints.ai_flags = AI_PASSIVE;    /* For wildcard IP address */
  hints.ai_protocol = 0;          /* Any protocol */
  hints.ai_canonname = NULL;
  hints.ai_addr = NULL;
  hints.ai_next = NULL;

  obtainAddresses(NULL, argv[1], &hints, &result);
  if (result) {
    bindAddress(result, &sfd);
  } else {
    fprintf(stderr, "Could not obtain any address\n");
    exit(EXIT_FAILURE);
  }

/* #define ALLOC_SIZE 2048
  DataSource filetoSend = DataSource(argv[2], sourceMode::IN);
  struct DataFactory txFac = {.buffer =
                                  threadsafe_queue<struct memBuff>(ALLOC_SIZE),
                              .allocSize = ALLOC_SIZE,
                              .curSize = 0,
                              .curPos = 0,
                              .curWritePos = 0,
                              .curReadPos = 0,
                              .chunkSize = 1480,
                              .sourceData = &filetoSend};
  struct DataFactory rxFac = {.buffer =
                                  threadsafe_queue<struct memBuff>(ALLOC_SIZE),
                              .allocSize = ALLOC_SIZE,
                              .curSize = 0,
                              .curPos = 0,
                              .curWritePos = 0,
                              .curReadPos = 0,
                              .chunkSize = 1480,
                              .sourceData = NULL};
  // datautil.setDataSource(filetoSend, &datautil);
  Pipeline::gateway = std::unique_ptr<Pipeline::Gateway>(new Pipeline::Gateway(
      sfd, peer_addr, peer_addrlen, result, &txFac, &rxFac));
  Pipeline::session = std::unique_ptr<Pipeline::Session>(
      new Pipeline::Session(stateDirection::TX, &txFac, &rxFac));
  Pipeline::compositor = std::unique_ptr<Pipeline::Compositor>(
      new Pipeline::Compositor(stateDirection::TX, &txFac, &rxFac));
  assert(Pipeline::compositor.get());
  assert(Pipeline::session.get());
  assert(Pipeline::gateway.get());
  // Pipeline::Session clientSession(sfd, &peer_addrlen, &peer_addr,
  // &datautil);
  for (;;) {
    Pipeline::session->start();
    Pipeline::session->destroy();
    fprintf(stdout, "sending to client done\nAccepting new connections\n");
    Pipeline::session->set_next(stateCodes::open);
    break;
  }
  destroyDataUtil(&txFac);
  destroyDataUtil(&rxFac); */
  exit(EXIT_SUCCESS);
}

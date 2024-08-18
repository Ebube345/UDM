#include <cstddef>
#include <string>
#include "worker_pool.hpp"

#include "connection.h"
#include "session.h"
namespace udm {
typedef int sockfd_t;
typedef short port_t;
enum error_code { eof = 0, conn_close };
enum ipv { v4 = 0, v6 };
enum role { TX = 0, RX };
#define ALLOC_SIZE 49152

class io_context {
 public:
  std::unique_ptr<Pipeline::Gateway> &gatewayref;
  std::unique_ptr<Pipeline::Session> &sessionref;
  std::unique_ptr<Pipeline::Compositor> &compositorref;
private:
  struct sockaddr_storage peer_addr;
  socklen_t peer_addrlen = sizeof(peer_addr);
  struct addrinfo *srv_result, *cl_result, *rp;
  int sfd;
  std::unique_ptr<DataFactory> txFac, rxFac;
  stateDirection dir;


 public:
  io_context(stateDirection i_dir)
      : gatewayref(Pipeline::gateway),
        sessionref(Pipeline::session),
        compositorref(Pipeline::compositor),
        txFac(new DataFactory(ALLOC_SIZE)),
        rxFac(new DataFactory(ALLOC_SIZE)),
        dir(i_dir){};
  void setfd(sockfd_t fd) { sfd = fd; }
  sockfd_t getfd() { return sfd; }
  void run() {
    gatewayref = std::unique_ptr<Pipeline::Gateway>(new Pipeline::Gateway(
        sfd, peer_addr, peer_addrlen, srv_result, txFac.get(), rxFac.get()));
    sessionref = std::unique_ptr<Pipeline::Session>(
        new Pipeline::Session(dir, txFac.get(), rxFac.get()));
    compositorref = std::unique_ptr<Pipeline::Compositor>(
        new Pipeline::Compositor(dir, txFac.get(), rxFac.get()));
  }
  struct sockaddr_storage &get_peer() { return peer_addr; }
  void set_peer(struct sockaddr_storage p) { peer_addr = p; }
  void set_source(DataSource *s) { txFac->sourceData = s; };
  void set_sink(DataSource *s) { rxFac->sourceData = s; };
  ~io_context(){};
};
class contextManager{
 public:
  contextManager() = default;
  contextManager(const contextManager &) = default;
  contextManager(contextManager &&) = default;
  contextManager &operator=(const contextManager &) = default;
  contextManager &operator=(contextManager &&) = default;
};

struct endpoint {
  int en_flags;             /* Input flags.  */
  int en_family;            /* Protocol family for socket.  */
  int en_socktype;          /* Socket type.  */
  int en_protocol;          /* Protocol for socket.  */
  socklen_t en_addrlen;     /* Length of socket address.  */
  struct sockaddr *en_addr; /* Socket address for socket.  */
  char *en_canonname;       /* Canonical name for service location.  */
  struct addrinfo *en_next; /* Pointer to next in list.  */
};
class resolver {
  io_context &contref;

 public:
  resolver(io_context &i_cont) : contref(i_cont) {}
  ~resolver() {}
  void listing(){};
  /*
   * Get the ip addresses of the specified host and port pair
   * */
  endpoint resolve(std::string &ipaddr, std::string & service) {
    auto ip(std::make_unique<char[]>(ipaddr.size()));
    auto srv(std::make_unique<char[]>(service.size()));
    //sprintf(strport, "%u", port);
    strcpy(ip.get(), ipaddr.c_str());
    strcpy(srv.get(), service.c_str());
    struct addrinfo hints;
    struct addrinfo *result;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;    /* Allow IPv4 or IPv6 */
    hints.ai_socktype = SOCK_DGRAM; /* Datagram socket */
    hints.ai_flags = 0;
    hints.ai_protocol = 0; /* Any protocol */
    obtainAddresses(ip.get(), srv.get(), &hints, &result);
    auto peer_addr = contref.get_peer();
    memcpy(&peer_addr, (struct sockaddr_storage *)result->ai_addr,
           sizeof(peer_addr));
    contref.set_peer(peer_addr);
    return *(endpoint *)result;
    // convert result to endpoint
    // set io_context addr
  };
};
class acceptor {
  io_context &contref;
  endpoint addr;

 public:
  acceptor(io_context &i_cont, endpoint &ep) : contref(i_cont), addr(ep){};
  ~acceptor(){};
  void accept(sockfd_t &){};
};
class source {
  size_t len = 0;
  DataSource _src;

 public:
  source(char *name, sourceMode mode) : _src(name, mode){};
  ~source(){};
  DataSource *get() { return &_src; }
};
class buffer {
  io_context &cont;
  std::shared_ptr<DataFactory> factory;

 public:
  buffer(io_context &i_cont, std::string &, size_t) : cont(i_cont){};
  buffer(io_context &i_cont) : cont(i_cont){};
  ~buffer(){};
  bool set_source(source &src) {
    cont.set_source(src.get());
    return false;
  };
  bool load(source &) { return false; };
  bool set_sink(source &src) {
    cont.set_sink(src.get());
    return false;
  };
  bool reload(source &, size_t) { return false; };
  bool add(source &, size_t) { return false; };
  void drain() {}
};
class u_socket {
 public:
  u_socket(io_context &, sockfd_t &){

  };
  ~u_socket(){};
};
struct error {
  error_code code;
  std::string reason;

 public:
  error(){};
  ~error(){};
  error_code eof() { return error_code::eof; };
};
class generic_error : std::exception {
  std::string reason;

 public:
  generic_error(std::string &message) : reason{message} {};
  virtual const char *what() const noexcept override { return reason.c_str(); };
};

void udm_socket(io_context &cont, sockfd_t &sfd, endpoint &ep) {
  struct addrinfo *rp;
  for (rp = (struct addrinfo *)&ep; rp != NULL; rp = rp->ai_next) {
    sfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
    if (sfd == -1) continue;
    break;
    close(sfd);
  }
  cont.setfd(sfd);
  if (rp == NULL) { /* No address succeeded */
    fprintf(stderr, "Could not connect\n");
    exit(EXIT_FAILURE);
  }
};
void udm_bind(io_context& contref, endpoint& addr){
  auto sfd = contref.getfd();
  if (sfd > 0)
    bindAddress((struct addrinfo *)&addr, &sfd);
}
void udm_teardown();
size_t udm_send(buffer &, error &);
void udm_txstart(io_context *cont, buffer &, error &) {
  cont->run();
  assert(cont->compositorref.get());
  assert(cont->sessionref.get());
  assert(cont->gatewayref.get());
  cont->sessionref->start();
  cont->sessionref->destroy();
}
void udm_rxstart(io_context *cont, buffer &, error &ec) {
  cont->run();
  assert(cont->compositorref.get());
  assert(cont->sessionref.get());
  assert(cont->gatewayref.get());
  Pipeline::session->start();
  Pipeline::session->destroy();
  // TODO: ERRORS
  if (/* SUCCESS */ 1) {
    ec.code = error_code::eof;
  } else {
    ec.code = /* DEPENDS ON THE ERROR */ error_code::conn_close;
  }
}
void udm_recv();
void udm_setTxopts();
void udm_setRxopts();
void cleanup(io_context &) noexcept {};
void udm_connect(sockfd_t, endpoint &);
endpoint fetch_addresses(ipv, port_t port) {
  struct addrinfo hints;
  struct addrinfo *result;
  std::string p_str = std::to_string(port);
  auto p = std::make_unique<char[]>(p_str.size());
  strcpy(p.get(), p_str.c_str());
  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_UNSPEC;    /* Allow IPv4 or IPv6 */
  hints.ai_socktype = SOCK_DGRAM; /* Datagram socket */
  hints.ai_flags = AI_PASSIVE;    /* For wildcard IP address */
  hints.ai_protocol = 0;          /* Any protocol */
  hints.ai_canonname = NULL;
  hints.ai_addr = NULL;
  hints.ai_next = NULL;
  obtainAddresses(NULL, p.get(), &hints, &result);
  return *(endpoint *)result;
};
}  // namespace udm

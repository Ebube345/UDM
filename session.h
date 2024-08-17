#ifndef SESSION_H
#define SESSION_H

#include <netdb.h>

#include <atomic>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <iostream>
#include <map>
#include <memory>

#include "datafactory.h"
#include "gateway.h"
#include "presentation.h"
#include "stdutils.h"

#define WIN_SIZ 3
namespace Pipeline {
std::map<uint16_t, memBuff *> rxcurrentWindow;
std::map<int, memBuff *> txcurrentWindow;
class Session;
std::unique_ptr<Compositor> compositor;
std::unique_ptr<Gateway> gateway;
std::unique_ptr<Session> session;
typedef void (*state)(int, stateDirection, struct memBuff *);
static constexpr uint8_t STATES_COUNT = 8;
static void openState(int nextId, stateDirection dir, struct memBuff *);
static void setupState(int nextId, stateDirection dir, struct memBuff *);
static void connectedState(int nextId, stateDirection dir, struct memBuff *);
static void closedState(int nextId, stateDirection dir, struct memBuff *);
static void doneState(int nextId, stateDirection dir, struct memBuff *);
static void inqState(int nextId, stateDirection dir, struct memBuff *);
static void sendState(int nextId, stateDirection dir, struct memBuff *);
static void receiveState(int nextId, stateDirection dir, struct memBuff *);
uint8_t genSrcId() {
  static uint8_t prevSrcid = 0;
  return ++prevSrcid;
};
uint8_t genDestId() {
  static uint8_t prevDstid = 0;
  return ++prevDstid;
};
uint16_t genConnId() {
  static uint16_t prevConnid = 0;
  return ++prevConnid;
};

struct connection {
  uint16_t id;
  uint8_t src;
  uint8_t dst;
  uint8_t status;
};
#define MAX_CONN_SIZ 256
struct contentNegotiation {};
class SessionProcessor {
  int capsuleId = 0;

 public:
  void _setFlags();

 public:
  uint16_t winSize = 0;

 public:
  SessionProcessor() {};
  ~SessionProcessor(){};
  void setCapParams(capsule *cap, struct params *p, stateCodes code) {
    if (cap) {
      switch (code) {
        case stateCodes::open: {
          break;
        }
        case stateCodes::setup: {
          // 1. save the peer id and set the connection id
          // 2. send an array of all the interfaces available including the one
          // that the client is currently connected to
          int addrlistLen = ADDR_LEN * gateway->interfaces.getInfCount();
          // fprintf(stdout, "addr list len = %d\n", addrlistLen);
          //char addresses[addrlistLen];
          // copy that into the body of the capsule
          auto interfaces = gateway->interfaces.getInterfaces();
          for (int i = 0; i < gateway->interfaces.getInfCount(); i++) {
            memcpy(cap->body + HEADER_SPACE + (i * ADDR_LEN),
                   interfaces[i].data, ADDR_LEN);
            //_LOG(interfaces[i].data);
          }
          cap->header.capSize = addrlistLen;
          break;
        }
        case stateCodes::connected: {
          /* 1. set the connection id for this connection ( the srcid is
           * generated for the first capsule that was sent i.e the open state
           * capsule
           * 2. set the dest(peer) id for this connection
           * 3. set the window size for this connection
           *  */
          cap->header.capWinSiz = WIN_SIZ;
          winSize = cap->header.capWinSiz;
          int addrlistLen = ADDR_LEN * gateway->interfaces.getInfCount();
          // fprintf(stdout, "addr list len = %d\n", addrlistLen);
          //char addresses[addrlistLen];
          // copy that into the body of the capsule
          auto interfaces = gateway->interfaces.getInterfaces();
          for (int i = 0; i < gateway->interfaces.getInfCount(); i++) {
            memcpy(cap->body + HEADER_SPACE + (i * ADDR_LEN),
                   interfaces[i].data, ADDR_LEN);
            //_LOG(interfaces[i].data);
          }
          cap->header.capSize = addrlistLen;
          break;
        }
        case stateCodes::closed: {
          if (cap->header.capFlags & (uint8_t)flags::FIN) {
            // set the connection status to the FIN state
          }
          break;
        }
        case stateCodes::done: {
          cap->header.capWinSiz = WIN_SIZ;
          break;
        }
        case stateCodes::send: {
          break;
        }
        case stateCodes::recv: {
          break;
        }
        default:
          break;
      };
      /* NOTE: These header fields must be set for all kinds of capsules
          struct params {
            1. uint8_t srcid;  *
            2. uint8_t destid; *
            3. uint16_t connid; *
            4. uint16_t capid; *
            5. uint16_t capSize; *
            6. uint16_t capType : 2; *
            7. uint16_t capPriority : 2; *
            8. uint16_t capWinSiz : 4; // in bytes *
            9. uint16_t capFlags : 8;
            10. uint32_t capopts;
          };
      */
      cap->header.srcid = p->srcid;
      cap->header.destid = p->destid;
      cap->header.connid = p->connid;
      cap->header.capid = capsuleId++;
      cap->header.capPriority = 0;
      cap->header.capType = (uint16_t)p->capType;
      // cap->header.capFlags = (uint8_t)p->capFlags;
      //_LOG(cap->header.capSize);
    }
  }
  void process(DataFactory *factory, struct memBuff *buf,
               stateCodes code)  {
    if ((buf->data->header.capType & (uint16_t)capsuleType::CTRL) == 0) {
      switch (code) {
        case stateCodes::open: {
          break;
        }
        case stateCodes::setup: {
          // process the body and convert it to an addrlist
          int noOfAddresses = (buf->data->header.capSize) / ADDR_LEN;
          gateway->interfaces.peer_addressCount = noOfAddresses;
          gateway->interfaces.peer_addressList = (struct ipaddr_dummy *)malloc(
              sizeof(struct ipaddr_dummy) * noOfAddresses);
          for (int i = 0; i < noOfAddresses; i++) {
            memcpy(gateway->interfaces.peer_addressList[i].data,
                   buf->data->body + (i * ADDR_LEN), ADDR_LEN);
            //_LOG(gateway->interfaces.peer_addressList[i].data);
          }

          // remove the loopback address if the peer is not the host
          bool localhost_found = false;
          for (int i = 0; i < noOfAddresses; i++) {
            if (strcmp(gateway->interfaces.peer_addressList[i].data,
                       "127.0.0.1") == 0) {
              // TODO: check that the peer is not localhost then remove that
              // address
              localhost_found = true;
              gateway->interfaces.peer_addressCount--;
            }
            if (localhost_found && i < noOfAddresses) {
              gateway->interfaces.peer_addressList[i] =
                  gateway->interfaces.peer_addressList[i + 1];
            }
          }
          //_LOG(buf->data->header.capSize);
          break;
        }
        case stateCodes::connected: {
          // extract the window size
          winSize = buf->data->header.capWinSiz;
          // printf("window size = %d\n", winSize);
          int noOfAddresses = (buf->data->header.capSize) / ADDR_LEN;
          gateway->interfaces.peer_addressCount = noOfAddresses;
          gateway->interfaces.peer_addressList = (struct ipaddr_dummy *)malloc(
              sizeof(struct ipaddr_dummy) * noOfAddresses);
          for (int i = 0; i < noOfAddresses; i++) {
            memcpy(gateway->interfaces.peer_addressList[i].data,
                   buf->data->body + (i * ADDR_LEN), ADDR_LEN);
            //_LOG(gateway->interfaces.peer_addressList[i].data);
          }
          bool localhost_found = false;
          for (int i = 0; i < noOfAddresses; i++) {
            if (strcmp(gateway->interfaces.peer_addressList[i].data,
                       "127.0.0.1") == 0) {
              // TODO: check that the peer is not localhost then remove that
              // address
              //_LOG("found localhost");
              localhost_found = true;
              gateway->interfaces.peer_addressCount--;
            }
            if (localhost_found && i < noOfAddresses) {
              gateway->interfaces.peer_addressList[i] =
                  gateway->interfaces.peer_addressList[i + 1];
            }
          }
          //_LOG(buf->data->header.capSize);
          break;
        }
        case stateCodes::closed: {
          //_LOG((uint16_t)buf->data->header.capFlags);
          if (buf->data->header.capType & (uint8_t)capsuleType::PAYLD) {
            rxcurrentWindow[buf->data->header.capid] = buf;
          }
          if (buf->data->header.capFlags & (uint8_t)flags::FIN) {
            // set the connection status to the FIN state
            factory->fin.value = true;
            _LOG("FIN SET");
          }
          break;
        }
        case stateCodes::inq: {
          break;
        }
        case stateCodes::send: {
          break;
        }
        case stateCodes::recv: {
          break;
        }
        default:
          break;
      };
    }
  };
  void stripHeader() {};
};
class Session {
  state states[STATES_COUNT];
  stateDirection Dir;

 public:
  std::map<int, connection>
      connections;  // table of connections attached to this session
  int connection_count = 0;
  std::atomic_bool finset;

 public:
  bool complete = false;
  void start() {
    std::unique_ptr<struct memBuff> newBuff = std::make_unique<struct memBuff>();
    if (next == stateCodes::done) {
      set_next(stateCodes::connected);
    } else {
    }
    while (next != stateCodes::connected || complete != true) {
      // assert(next!= -1)
      states[(int)next]((int)next, Dir,
                        newBuff.get());  // start from 0 i.e open state
      // next = static_cast<stateCodes>(static_cast<int>(next) + 1);
    }  // only the done states sets the done flags
  };
  void destroy() {
    //_LOG("CLEANING UP USED RESOURCES\n");
    txFactory->destroy();
    rxFactory->destroy();
  }
  std::unique_ptr<SessionProcessor> processor =
      std::unique_ptr<SessionProcessor>(new SessionProcessor());
  void set_next(stateCodes newnext) { next = newnext; }
  std::atomic_bool &is_done() { return done; }
  Session() = delete;
  Session(stateDirection dir, struct DataFactory *tx, struct DataFactory *rx)
      : Dir(dir), txFactory(tx), rxFactory(rx) {
    _init();
  }
  template <typename FunctionType>
  void submit(FunctionType f) {
    pool.submit(f);
  }

 public:
  bool workersDone() { return pool.workdone(); }

 private:
  thread_pool pool;

 private:
  stateCodes next = stateCodes::open;
  std::atomic_bool done = false;
  void _init() {
    states[(int)stateCodes::open] = (state)openState;
    states[(int)stateCodes::setup] = (state)setupState;
    states[(int)stateCodes::connected] = (state)connectedState;
    states[(int)stateCodes::closed] = (state)closedState;
    states[(int)stateCodes::done] = (state)doneState;
    states[(int)stateCodes::inq] = (state)inqState;
    states[(int)stateCodes::send] = (state)sendState;
    states[(int)stateCodes::recv] = (state)receiveState;
  }

 public:
  struct DataFactory *txFactory;
  struct DataFactory *rxFactory;
  uint16_t connId;
  // struct memBuff *currentBuffer;
  std::shared_ptr<struct memBuff> currentMemBuff;
};  // namespace Session
void submitTask(int nextId, stateDirection dir, struct memBuff *);
static void openState(int nextId, stateDirection dir,
                      struct memBuff *currentBuffer) {
  //_LOG("OPEN");
  switch (dir) {
    case stateDirection::TX: {
      //_LOG("EXPECTING HELLO CAPSULE");
      session->connections[session->connection_count++] =
          connection{.id = 0, .src = genSrcId(), .dst = 0, .status = 0};
      currentBuffer = compositor->newBuffer(stateDirection::RX);
      receiveState(nextId, dir, currentBuffer);
      auto buf = session->rxFactory->buffer.try_pop();
      currentBuffer = buf.get();
      session->processor->process(session->rxFactory, currentBuffer,
                                  stateCodes::open);
      // TX: save the dst id (i.e the src id of the capsule)
      session->connections[session->connection_count].dst =
          currentBuffer->data->header.srcid;
      break;
    }
    case stateDirection::RX: {
      //_LOG("SENDING HELLO CAPSULE");
      currentBuffer = compositor->newBuffer(stateDirection::TX);
      // the dstid of the capsule would be set by the processor->process when we
      // rx a capsule from the peer
      session->connections[session->connection_count++] =
          connection{.id = 0, .src = genSrcId(), .dst = 0, .status = 0};
      struct params p {
        .srcid = session->connections[session->connection_count].src,
        .destid = session->connections[session->connection_count].dst,
        .connid = session->connections[session->connection_count].id,
        .capid = 0,
        .capSize = 0,
        .capType = (uint16_t)capsuleType::PAYLD,
        .capPriority = 0,
        .capWinSiz = 0,
        .capFlags = 0,
        .capopts = 0
      };
      session->processor->setCapParams(currentBuffer->data.get(), &p,
                                       stateCodes::open);
      sendState(nextId, dir, currentBuffer);
      auto buf = session->txFactory->buffer.try_pop();
      break;
    }
    default:
      break;
  }
  session->set_next(stateCodes::setup);
};
static void setupState(int nextId, stateDirection dir,
                       struct memBuff *currentBuffer) {
  //_LOG("SETUP");
  switch (dir) {
    case stateDirection::TX: {
      //_LOG("SENDING Hi CAPSULE WITH ALONG INTERFACE INFO");
      currentBuffer = compositor->newBuffer(stateDirection::TX);
      struct params p {
        .srcid = session->connections[session->connection_count].src,
        .destid = session->connections[session->connection_count].dst,
        .connid = session->connections[session->connection_count].id,
        .capid = 0,
        .capSize = 0,
        .capType = (uint16_t)capsuleType::CTRL,
        .capPriority = 0,
        .capWinSiz = 0,
        .capFlags = 0,
        .capopts = 0
      };
      session->processor->setCapParams(currentBuffer->data.get(), &p,
                                       stateCodes::setup);
      sendState(nextId, dir, currentBuffer);  // sending a response hi
      session->txFactory->buffer.try_pop();
      break;
    }
    case stateDirection::RX: {
      receiveState(nextId, dir, currentBuffer);
      auto buf = session->rxFactory->buffer.try_pop();
      currentBuffer = buf.get();
      session->processor->process(session->rxFactory, currentBuffer,
                                  stateCodes::setup);
      session->connections[session->connection_count].dst =
          currentBuffer->data->header.srcid;
      break;
    }
    default:
      break;
  }
  session->set_next(stateCodes::connected);
};
static void connectedState(int nextId, stateDirection dir,
                           struct memBuff *currentBuffer) {
  //_LOG("CONNECTED");
  // add the connection to the connections array after the ACK - by setting a
  // valid connid i.e not 0
  switch (dir) {
    case stateDirection::TX: {
      // the expecting ack should come with a ping of the new subconnection(s)
      //_LOG("EXPECTING AN ACK CAP");
      receiveState(nextId, dir, currentBuffer);
      auto buf = session->rxFactory->buffer.try_pop();
      currentBuffer = buf.get();
      session->processor->process(session->rxFactory, currentBuffer,
                                  stateCodes::connected);
      session->connections[session->connection_count].id =
          currentBuffer->data->header.connid;  // connection setup complete
      break;
    }
    case stateDirection::RX: {
      //_LOG("SENDING AN ACK CAP");
      // NOTE: this is for the primary connection - a seperate connection object
      // must be setup for the secondary connnection(s) also
      session->connections[session->connection_count].id = genConnId();
      currentBuffer = compositor->newBuffer(stateDirection::TX);
      struct params p {
        .srcid = session->connections[session->connection_count].src,
        .destid = session->connections[session->connection_count].dst,
        .connid = session->connections[session->connection_count].id,
        .capid = 0,
        .capSize = 0,
        .capType = (uint16_t)capsuleType::CTRL,
        .capPriority = 0,
        .capWinSiz = 0,
        .capFlags = 0,
        .capopts = 0
      };
      session->processor->setCapParams(currentBuffer->data.get(), &p,
                                       stateCodes::connected);
      sendState(nextId, dir, currentBuffer);
      session->txFactory->buffer.try_pop();
      break;
    }
    default:
      break;
  }
  session->set_next(stateCodes::closed);
};

// the closed state should empty the TX buffer
static void closedState(int nextId, stateDirection dir,
                        struct memBuff *currentBuffer) {
  //_LOG("CLOSED");
  // send data -  Window size at a time as received from the client
  // Pick a valid connection from the connection table of the session
  // send window to that connection
  // NOTE: Pick a connection and send window on the connection
  // NOTE: A Window sent on a connection must be ACKed on the same connection
  // ideally each connection should have a seperate interface but this is not
  // mandatory
  while (session->processor->winSize > 0) {
    //session->submit([&nextId, &dir, &currentBuffer]() {
      submitTask(nextId, dir, currentBuffer);
    //});
    if (session->processor->winSize) session->processor->winSize--;
    // fprintf(stdout, "window size = %d\n", (int)session->processor->winSize);
  }
  while (!session->workersDone()) {
  }
  //_LOG("WORKERS ARE DONE");
  session->set_next(stateCodes::done);
};
static void doneState(int nextId, stateDirection dir,
                      struct memBuff *currentBuffer) {
  //_LOG("DONE");
  switch (dir) {
    case stateDirection::TX: {
      //_LOG("EXPECTING CTRL OK CAPSULE\n");
      receiveState(nextId, dir, currentBuffer);
      auto buf = session->rxFactory->buffer.try_pop();
      currentBuffer = buf.get();
      session->processor->process(session->rxFactory, currentBuffer,
                                  stateCodes::done);
      if (session->txFactory->fin.value) {
        session->set_next(stateCodes::connected);
      } else {
        // reset the window size
        session->processor->winSize = WIN_SIZ;
        session->set_next(stateCodes::closed);
      }
      break;
    }
    case stateDirection::RX: {
      // generate an ok capsule
      //_LOG("SENDING CTRL OK CAPSULE\n");
      bool bufferempty = false;
      if (session->rxFactory->fin.value) {  // now we need to change how we check if we
                                  // received our last packet since we can
                                  // receive the last packet before another
                                  // packet (due to multithreading)
        bufferempty = true;
        _LOG("FIN ACKed");
      }
      compositor->processor->process(
          session->rxFactory,
          rxcurrentWindow, stateCodes::done);
      currentBuffer = compositor->newBuffer(stateDirection::TX);
      for (int i = 0; i < (int)rxcurrentWindow.size(); i++) {
        session->rxFactory->buffer.try_pop();
      }
      rxcurrentWindow.clear();
      struct params p {
        .srcid = session->connections[session->connection_count].src,
        .destid = session->connections[session->connection_count].dst,
        .connid = session->connections[session->connection_count].id,
        .capid = 0,
        .capSize = 0,
        .capType = (uint16_t)capsuleType::CTRL,
        .capPriority = 0,
        .capWinSiz = 0,
        .capFlags = 0,
        .capopts = 0
      };
      if (bufferempty) p.capFlags = (uint8_t)flags::FIN;
      session->processor->setCapParams(currentBuffer->data.get(), &p,
                                       stateCodes::done);
      sendState(nextId, dir, currentBuffer);
      if (session->rxFactory->fin.value) {
        session->set_next(stateCodes::connected);
      } else {
        // reset the window size
        session->processor->winSize = WIN_SIZ;
        session->set_next(stateCodes::closed);
      }
      break;
    }
    default:
      break;
  }
  session->is_done() = true;
  session->complete = true;
};
static void inqState(int nextId, stateDirection dir,
                     struct memBuff *currentBuffer){
    //_LOG("INQ");
  if (currentBuffer){}
  if (nextId){}
    switch (dir) {
    case stateDirection::TX:
      break;
    case stateDirection::RX:
      break;
    default:
      break;
    }
};
static void sendState(int nextId, stateDirection dir,
                      struct memBuff *currentBuffer) {
  //_LOG("SEND");
  if (nextId == (int)stateCodes::open && dir == stateDirection::RX) {
    gateway->chunkWrite(currentBuffer->data.get());
  } else if (dir == stateDirection::RX) {
    // currentBuffer = gateway->push();
    gateway->chunkWrite(currentBuffer->data.get());
  } else {
    gateway->send(currentBuffer->data.get());
  }
  // if send was successfull transition to the next state by default
  // test would be added at the right time
};
static void receiveState(int nextId, stateDirection dir,
                         struct memBuff *currentBuffer) {
  //_LOG("RECV");
  if (dir == stateDirection::TX){}
  if (nextId == (int)stateCodes::open) {
    gateway->receive(currentBuffer);
  } else {
    struct memBuff * res = gateway->pull();
    if (res->data.get() == nullptr) {
        perror("no data");
        exit(1);
    }
    if (res->data->header.capType & (uint8_t)capsuleType::PAYLD){
      rxcurrentWindow[res->data->header.capid] = res;
    }
    else {
    /* currentBuffer->data = std::make_unique<capsule>();
    currentBuffer->size = res->size;
    currentBuffer->data = std::move(res->data); */
    }
  }
};
void submitTask(int nextId, stateDirection dir, struct memBuff *currentBuffer) {
  switch (dir) {
    case stateDirection::TX: {
      //_LOG("SENDING PAYLOAD");
      session->txFactory->updateDataSource();
      auto currentMemBuff =
          compositor->constructChunk();  // constructchunk ok
      auto currentBuff = currentMemBuff.get();
      if (session->txFactory->fin.value && session->txFactory->buffer.isEmpty()) {
        currentBuff->data->header.capFlags = (uint8_t)flags::FIN;
        session->processor->winSize = 0;
      }
      struct params p {
        .srcid = session->connections[session->connection_count].src,
        .destid = session->connections[session->connection_count].dst,
        .connid = session->connections[session->connection_count].id,
        .capid = 0,
        .capSize = 0,
        .capType = (uint16_t)capsuleType::PAYLD,
        .capPriority = 0,
        .capWinSiz = 0,
        .capFlags = 0,
        .capopts = 0
      };
      compositor->processor->setCapParams(currentBuff->data.get(), &p,
                                          stateCodes::closed);
      session->processor->setCapParams(currentBuff->data.get(), &p,
                                       stateCodes::closed);
      /* _LOG(currentBuff);
      _LOG(currentBuff->data->header.capid); */
    /* session->submit([&nextId, &dir, data]() {
      _LOG(data->header.capid); */
      /* _LOG(currentBuff);
        _LOG('x'); */
      sendState(nextId, dir, currentBuff);
/*       }); */
      break;
    }
    case stateDirection::RX: {
      //_LOG("EXPECTING PAYLOAD");
      std::shared_ptr<struct memBuff> buf = std::make_unique<struct memBuff>();
      struct memBuff *currentBuff = buf.get();
      _LOG(currentBuff->data);
/*     session->submit([&nextId, &dir, &currentBuff]() { */
      receiveState(nextId, dir, currentBuff);
/*     }); */
      //buf = session->rxFactory->buffer.try_pop();
      if (rxcurrentWindow.rbegin() != rxcurrentWindow.rend()){
        currentBuff = rxcurrentWindow.rbegin()->second;
      }
      else {
        _LOG("Buffer not found");
        break;
      }
      session->processor->process(session->rxFactory, currentBuff,
                                  stateCodes::closed);
      if (session->rxFactory->fin.value == true) {
        _LOG("FIN ACTIVE");
        session->processor->winSize = 0;
      }
      if (currentBuff->data->header.capFlags & (uint8_t)flags::FIN) {
       if (currentBuffer->data) {
        currentBuffer->data->header.capFlags = (uint8_t)flags::FIN;
       }
      }
      break;
    }
    default:
      break;
  }
}
};  // namespace Pipeline
// A hybrid datastructure that behaves like a queue for the insertion and
// deletion and behaves like a random access array for access (read&write)
#endif

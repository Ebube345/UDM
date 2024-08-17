#ifndef DATAFACTORY_H
#define DATAFACTORY_H
// #include "LZSS.h"
#include <stdio.h>

#include <filesystem>
#include <fstream>
#include <string>

#include "circular_pipeline.hpp"
#include "stdutils.h"
#define HEADER_SPACE sizeof(capsuleHdr)
#define CHUNK 1024

enum class sourceMode { IN = 0x8, OUT = 0x10 };
enum class capsuleType { CTRL = 0b00, PIGGY = 0b01, INQ = 0b10, PAYLD = 0b11 };
/*
 * There is room for only 8 flags
 * Flags Explanation
 * 1. FIN - Last Buffer of Last Window has been sent
 * 2. DON - Last Buffer of a Window has been sent
 * 3. RST - End of a Connection
 * 4. ACK - Acknlowledgement
 * 5. OK  -  Window received was intact
 * */
enum class flags {
  FIN = 0b0000001,
  DON = 0b0000010,
  OK = 0b0000100,
  RST = 0b0001000,
  ACK = 0b0010000
};

enum class action { SEND, RECV, QUIT, RELOAD, READ, WRITE };

enum class stateCodes {
  open = 0x0,
  setup,
  connected,
  closed,
  done,
  inq,
  send,
  recv
};
enum class stateDirection { TX = 0x0, RX };
struct canary {
  bool value = false;
};
typedef struct capsuleHdr {
  uint8_t srcid;
  uint8_t destid;
  uint16_t connid;
  uint16_t capid;
  uint16_t capSize;
  uint16_t capType : 2;
  uint16_t capPriority : 2;
  uint16_t capWinSiz : 4;  // in bytes
  uint16_t capFlags : 8;
  uint32_t capopts;
} __attribute__((__packed__)) capsuleHdr;
struct params {
  uint8_t srcid;
  uint8_t destid;
  uint16_t connid;
  uint16_t capid;
  uint16_t capSize;
  uint16_t capType : 2;
  uint16_t capPriority : 2;
  uint16_t capWinSiz : 4;  // in bytes
  uint16_t capFlags : 8;
  uint32_t capopts;
};
typedef struct capsule {
  capsuleHdr header;
  char *body;
} __attribute__((__packed__)) capsule;
typedef struct memBuff {
  int size;
 // std::mutex capLock;
  /* std::lock_guard<std::mutex> capLocker; */
  std::unique_ptr<capsule> data;
} memBuff;
class DataSource {
 public:
  int datahandle;
  sourceMode fmode;
  std::fstream fs;
  FILE *fptr;
  size_t fileSize = 0;
  void readFile();
  explicit DataSource(char *fname, sourceMode mode) : fmode(mode) {
    switch (mode) {
      case sourceMode::IN: {
        fs = std::fstream(fname, fs.in | fs.binary);
        fileSize = std::filesystem::file_size(fname);
        // printf("%s opened with size %ld\n", fname, fileSize);
        break;
      }
      case sourceMode::OUT: {
        fs = std::fstream{fname, fs.out | fs.binary};
        // fptr = fopen(fname, "wb");
        break;
        default:
          break;
      }
    }
    if (!fs.is_open()) {
      // fprintf(stderr, "unable to open file\n");
      // exit(EXIT_FAILURE);
    }
  };
  int getoutPos() { return fs.tellg(); }
  int getinPos() { return fs.tellp(); }
  void breakdown(){
      /* if (fs.is_open())
        fs.close(); */
  };
  DataSource() = default;
  ~DataSource(){
      // fclose(fptr);
  };
};
class DataFactory {
 public:
  DataFactory(uint siz) : buffer(siz), chunkSize(siz) {}
  threadsafe_queue<memBuff> buffer;
  struct canary fin;
  int allocSize = 0;
  int curSize = 0;
  int curPos = 0;
  int curWritePos = 0;
  int curReadPos = 0;
  int fpos = 0;
  int chunkSize = CHUNK;
  struct DataSource *sourceData = NULL;  // source or sink
  // returns the previous position offset and advances the position
  std::shared_ptr<memBuff> fetchChunk() {
    std::shared_ptr<memBuff> chunk;
    if ((chunk = std::move(buffer.try_pop()))) {
    } else {
      fprintf(stderr, "Buffer is NULL\n");
      exit(1);
    }
    return chunk;
  }
  char *genDummyData(char *buf, int size) {
    for (int i = 0; i < size; i++) {
      buf[i] = 'A';
    }
    return buf;
  }
  inline int getAmountRead(int amtBefore, int amtAfter) {
    return amtAfter - amtBefore;
  }
  // returns amount read
  int readStream(DataSource &data, char *buff, int size2Read) {
    int gcBefore = data.fs.gcount();
    data.fs.read(buff, size2Read);
    int gcAfter = data.fs.gcount() + gcBefore;
    //fprintf(stdout, "gcBefore = %d bytes, gcAfter = %d|  ", gcBefore, gcAfter);
    return getAmountRead(gcBefore, gcAfter);
  }
  int writeStream(DataSource &data, char *buff, int size2Write) {
    int gcBefore = data.fs.tellp();
    ////fprintf(stdout, "[%s] --> writing %d bytes\n", __func__, size2Write);
    data.fs.write(buff, size2Write);
    data.fs.flush();
    // fwrite(buff, size2Write, 1, data.fptr);
    int gcAfter = (int)data.fs.tellp();
    if (data.fs.tellp() < 0) {
      _LOG(data.fileSize);
      gcAfter = data.fileSize - 1;
    }
    // fprintf(stdout, "gcBefore = %d bytes, gcAfter = %d\n", gcBefore,
    // gcAfter);
    int amtwritten = getAmountRead(gcBefore, gcAfter);
    // fprintf(stdout, "[%s] --> amount actually written %d bytes\n", __func__,
    // amtwritten);
    // std::cout << std::filesystem::file_size("r2ebube.jpg") << "\n";
    // data.fs.flush();
    return amtwritten;
  }
  void prepareMemBuff(memBuff &capsuleHolder, std::unique_ptr<capsule> &cap,
                      DataFactory *datautil) {
    char *buf = (char *)malloc(sizeof(char) * (datautil->chunkSize));
    static uint hdrid = 0;
    cap->header.capid = hdrid++;
    cap->body = buf;
    capsuleHolder.data = std::move(cap);
  };
  struct memBuff *makeChunkforRx(DataFactory *datafac) {
    struct memBuff chunk;
    std::unique_ptr<capsule> cap = std::make_unique<capsule>();
    prepareMemBuff(chunk, cap, datafac);
    return datafac->buffer.pushWithRef(std::move(chunk));
  }
  void _inputFromSource(struct DataSource *data, DataFactory *datautil) {
    //_LOG("loading data from source");
    if (data->fmode == sourceMode::IN) {
      if (data->fileSize <= (size_t)datautil->curSize) {
        _LOG("FIN HAS BEEN SET");
        datautil->fin.value = true;
        return;
      }
      memBuff capsuleHolder;
      std::unique_ptr<capsule> cap = std::unique_ptr<capsule>(new capsule());
      //_LOG(datautil->chunkSize);
      size_t bodylen = datautil->chunkSize;
      cap->body = (char *)malloc(sizeof(char) * (bodylen));
      int readSize =
          readStream(*data, cap->body + HEADER_SPACE, bodylen - HEADER_SPACE);
      cap->header.capSize = readSize;
      if ((size_t)readSize < bodylen - HEADER_SPACE) {
        datautil->chunkSize = readSize + HEADER_SPACE;
        fprintf(stdout, "chunk size value changed to: %d|\t",
                datautil->chunkSize);
        datautil->fin.value = true;
      }
      capsuleHolder.data = std::move(cap);
      capsuleHolder.size = readSize + HEADER_SPACE;
      //fprintf(stdout, "capsule holder is of size %d\n", capsuleHolder.size);
      datautil->buffer.push(std::move(capsuleHolder));
      datautil->curSize += readSize;
    }
  }
  void initDataSource() {
    /* if (!datautil->data) {
      datautil->data = (char *)malloc(sizeof(char) * datautil->allocSize);
    }
    // nothing to do again for an output stream
    if (data.fmode != sourceMode::IN)
      return;
    int readSize = readStream(data, datautil->data, datautil->chunkSize);
    if (readSize < datautil->chunkSize) {
      datautil->chunkSize = readSize;
    }
    datautil->curReadPos += readSize;
    datautil->curSize = data.fs.tellg();
    printf("amount read = %d\n", (int)datautil->chunkSize); */
    _inputFromSource(sourceData, this);
  }

  void updateDataSource() {
    /* if (datafac->curSize >= data.fileSize) {
      datafac->fin.value = true;
      return;
    }
    if (!datafac->data) {
      //fprintf(stderr, "cannot update on a null source\n");
      exit(EXIT_FAILURE);
    }
    if (data.getinPos() == -1) {
      //fprintf(stderr, "cannot seek further\n");
      return;
    }
    if (datafac->curReadPos >= datafac->allocSize) {
      // start from the begining to use the buffer
      //fprintf(stdout,
              "reloading buffer with chunk size = %d and read position %d\n", //
              datafac->chunkSize, (int)data.fs.tellg());
      int readSize = readStream(data, datafac->data, datafac->chunkSize);
      datafac->curPos = 0;     // reset read position
      datafac->curReadPos = 0; // reset read position
      datafac->curReadPos += readSize;
      datafac->curSize += data.fs.gcount();
      if (readSize < datafac->chunkSize) {
        datafac->chunkSize = readSize;
        //fprintf(stdout, "new chunksize value = %d\n", datafac->chunkSize);
      }
      return;
    }
    // read some more data
    //fprintf(stdout, "updating buffer from read position %d\n",
            (int)data.fs.tellg());
    // prevent reading beyond allocated size
    int rem = datafac->allocSize - datafac->chunkSize;
    rem = rem > datafac->chunkSize ? datafac->chunkSize : rem;
    int readSize = readStream(data, datafac->data + datafac->curReadPos, rem);
    datafac->curReadPos += readSize;
    if (readSize < datafac->chunkSize) {
      datafac->chunkSize = readSize;
    }
    datafac->curSize += data.fs.gcount();
    //fprintf(stdout, "current position = %d\n", datafac->curReadPos); */
    _inputFromSource(sourceData, this);
  }
  void disposeAllocedBody(capsule *cap) {
    if (cap && cap->body) free(cap->body);
  }
  void write2Source(DataSource &data, struct memBuff *buf2Write,
                    int amt2Write) {
    if (data.fmode != sourceMode::OUT) return;
    /* std::shared_ptr<memBuff> chunk;
    if ((chunk = fetchChunk())) {
    } */
    int amtWritten =
        writeStream(data, buf2Write->data->body, amt2Write - HEADER_SPACE);
    curWritePos += amtWritten + HEADER_SPACE;
  }
  void destroy() { sourceData->breakdown(); }
};

class DataProcessor {
 public:
  DataProcessor() = default;
  virtual ~DataProcessor(){};
  virtual void setCapParams(capsule *buf, struct params *, stateCodes) = 0;
  virtual void stripHeader(capsule *buf) = 0;
  virtual void process(struct memBuff *, stateCodes) = 0;
};

namespace Pipeline {
  std::mutex memBuff_mtx;
  std::mutex datasource_mtx;
  std::mutex datafactory_mtx;
  std::mutex currentWindow_mtx;

}
#endif

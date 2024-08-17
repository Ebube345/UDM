#ifndef PRESENTATION_H
#define PRESENTATION_H

#include <cstdint>
#include <map>

#include "datafactory.h"
namespace Pipeline {
class Compressor {
 public:
  void compress(){};
  void decompress(){};
};
class Compositor;
class PresentationProcessor {
 public:
  PresentationProcessor(){};
  ~PresentationProcessor(){};
  void write2Souce(DataFactory *rxFactory, struct memBuff *chunk) {
    rxFactory->write2Source(*rxFactory->sourceData, chunk, chunk->size);
    rxFactory->curPos += chunk->size;
    rxFactory->curSize += chunk->size;
    if (rxFactory->curPos >= rxFactory->allocSize) rxFactory->curPos = 0;
    // fprintf(stdout, "Received %d bytes in total\n", rxFactory->curSize);
  }
  void process(DataFactory *factory,
               std::map<uint16_t, memBuff *> &currentWindow, stateCodes state) {
    switch (state) {
      case stateCodes::done :{
        /* for (int i = 0; i < currentWindow.size(); i++) {
          if (auto search = currentWindow.find(capHandle); search != currentWindow.end()){

          }
          if ((currentWindow[capHandle]->data->header.capType & (uint16_t)capsuleType::PAYLD) == 0) {
            _LOG("Skipping write... not payload data\n");
          } else {
            if (currentWindow[i+1])
              write2Souce(factory, currentWindow[i + 1]);
          }
        } */
        for (const auto &[key, windowBuffer] : currentWindow){
          if ((windowBuffer->data->header.capType & (uint16_t)capsuleType::PAYLD) == 0) {
            _LOG("Skipping write... not payload data\n");
          } else {
              write2Souce(factory, windowBuffer);
          }
        }
        break;
      }
      default:
        break;
    }
  }
  void setCapParams(capsule *buf, struct params *p, stateCodes)  {
    if (buf) {
      // set header parameters
      //...
      // copy header to the buffer body
      if (p->capType & (uint8_t)capsuleType::PAYLD) {
        // compress the payload and not the header
        /* std::string s(buf->body + HEADER_SPACE, buf->header.capSize -
        HEADER_SPACE), t; std::stringstream bufStream(s, std::ios_base::in);
        auto output = stl::OpenOutputBitFile(t);
        compressFile(bufStream, output);
        _LOG("Payload compression complete, size is now: ");
        auto newBufSize = output->file.source.str().size();
        _LOG(newBufSize);

        //Adjust the capsule size to fit the new size
        buf->header.capSize = newBufSize; */
      }
      memcpy(buf->body, (char *)&buf->header, HEADER_SPACE);
      // fprintf(stdout, "sending buffer at index [%u]\n", buf->header.capid);
    }
  }
  void stripHeader(capsule *buf)  {
    // recover header from the buffer body
    buf->header = *(capsuleHdr *)buf->body;
    // adjust buffer body after removing header
    buf->body = buf->body + sizeof(capsuleHdr);
    // fprintf(stdout,
    //"this buffer is at index [%u] and has a size of [%u] bytes\n",
    // buf->header.capid, buf->header.capSize);
    //}
  }
};
class Compositor {
 private:
  std::unique_ptr<Compressor> compressor;
  stateDirection Dir;
  struct DataFactory *txFactory;
  struct DataFactory *rxFactory;

 public:
  std::unique_ptr<PresentationProcessor> processor;

 public:
  Compositor() = delete;
  Compositor(stateDirection dir, struct DataFactory *tx, struct DataFactory *rx)
      : Dir(dir), txFactory(tx), rxFactory(rx) {
    compressor = std::unique_ptr<Compressor>(new Compressor());
    processor =
        std::unique_ptr<PresentationProcessor>(new PresentationProcessor());
    //if (Dir == stateDirection::TX) txFactory->initDataSource();
  };
  std::shared_ptr<struct memBuff> constructChunk() {
    return txFactory->fetchChunk();
  };
  struct memBuff *newBuffer(stateDirection dir) {
    struct memBuff * mbuff;
    switch (dir) {
      case stateDirection::TX: {
        mbuff = txFactory->makeChunkforRx(txFactory);
        mbuff->size = sizeof(struct capsule);
        break;
      }
      case stateDirection::RX: {
        mbuff = rxFactory->makeChunkforRx(rxFactory);
        mbuff->size = sizeof(struct capsule);
        break;
        }
      default:
        break;
    }
    return mbuff;
  };
};
}  // namespace Pipeline
#endif

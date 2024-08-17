#include <iostream>

#include "userclass.h"

int main(int argc, char* argv[]) {
  udm::io_context io_context(stateDirection::RX);
  do {
    try {
      if (argc != 4) {
        std::cerr << "Usage: client <host|ip> <service|port> <file>" << std::endl;
        return 1;
      }

      udm::resolver resolver(io_context);
      std::string addr(argv[1]);
      std::string service(argv[2]);
      udm::endpoint endpoints = resolver.resolve(addr,service );

      udm::sockfd_t fd;
      udm::udm_socket(io_context, fd, endpoints);
      if (fd <= 0) {
        printf("Unable to create socket\n");
        break;
      }
      // udm::udm_connect(fd, endpoints);
      for (;;) {
        std::string buf;
        //udm::error_code err_code;
        udm::error error;

        //char filename[] = "udm_ebube.jpg";
        udm::source file(argv[3], sourceMode::OUT);
        udm::buffer data(io_context);

        data.set_sink(file);
        auto start = std::chrono::high_resolution_clock::now();
        udm::udm_rxstart(&io_context, data, error);
        auto end =std::chrono::high_resolution_clock::now();
        auto time_diff = std::chrono::duration<float>(end - start).count();
        printf("Time elapsed = %fs\n", time_diff);
        if (error.code == udm::error_code::eof)
          break;  // Connection cloed cleanly by peer
        else if (error.code)
          throw udm::generic_error(error.reason);  // Some other error
        // std::cout.write(buf.c_str(), len);
        // file.put(data);
      }
    } catch (std::exception& e) {
      std::cerr << e.what() << std::endl;
      udm::cleanup(io_context);
    }
    udm::cleanup(io_context);
  } while (false);
  return 0;
}

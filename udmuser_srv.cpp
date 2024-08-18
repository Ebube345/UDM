#include <cstring>
#include <ctime>
#include <filesystem>
#include <iostream>
#include <string>

#include "userclass.h"

std::string make_daytime_string() {
  using namespace std;  // For time_t, time and ctime;
  time_t now = time(0);
  return ctime(&now);
}

int main(int argc, char* argv[]) {
  udm::io_context io_context(stateDirection::TX);
  do {
    try {
      if (argc < 2) {
        std::cerr << "Usage: server <service|port> [file]" << std::endl;
        return 1;
      }
      udm::port_t port = atoi(argv[1]);
      udm::sockfd_t fd;
      udm::endpoint addr = udm::fetch_addresses(udm::v4, port);
      udm::udm_socket(io_context, fd, addr);
      udm::udm_bind(io_context, addr);
      udm::acceptor acceptor(io_context, addr);

      for (;;) {
        acceptor.accept(fd);
        //udm::error_code err_code;
        udm::error error;
        char* filename;
        if (argc == 3) {
          filename = argv[2];
        } else {
          char defaul[] = "/home/jo/sandbox/outputs/ebube.jpg";
          strcpy(filename, defaul);
        }
        udm::source file(filename, sourceMode::IN);
        size_t filesize = std::filesystem::file_size(filename);
        _LOG(filesize);
        udm::buffer data(io_context);
        if (filesize > 0) {
          data.set_source(file);
        } else
          break;
        auto start = std::chrono::high_resolution_clock::now();
        udm::udm_txstart(&io_context, data, error);
        auto end = std::chrono::high_resolution_clock::now();
        auto time_diff = std::chrono::duration<float>(end - start).count();
        printf("Time elapsed = %fs\n", time_diff);
        break;
      }
    } catch (std::exception& e) {
      std::cerr << e.what() << std::endl;
      udm::cleanup(io_context);
    }
    udm::cleanup(io_context);
  } while (false);
  return 0;
}

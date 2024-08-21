This project contains an implementation of a UDP-based transport layer (4) protocol
This initial implementation is written in C/C++
This branch uses asio networking library to make the project cross compatible
follow these steps to build on a local machine:
* [Download Asio](https://think-async.com/Asio/Download.html) dependency and copy to the external folder (create the external folder if it does not exist)
* rename the asio-* to asio-latest (e.g from asio-1.28.0/ to asio-latest/)
* create a build/ directory in the source directory and run ```bash
cd build && cmake ..
make -j
```
*. All the executable binaries would be in the bin/ folder

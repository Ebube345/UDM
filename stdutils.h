#ifndef STDUTILS_H
#define STDUTILS_H
#include <syncstream>
#define _LOG(X)                                                                \
  (std::osyncstream(std::cout) << std::this_thread::get_id() <<  "[ " << __TIME__ << "] " << __func__ << " -> " << X << "\n")

#endif

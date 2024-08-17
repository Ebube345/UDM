#ifndef STDUTIL_H
#define STDUTIL_H

#define _LOG(X)                                                                \
  (std::cout << "[ " << __TIME__ << "] " << __func__ << " -> " << X << "\n")
#endif

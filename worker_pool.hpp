#ifndef WORKER_POOL_HPP
#define WORKER_POOL_HPP
#include "circular_pipeline.hpp"
#include <algorithm>
#include <atomic>
#include <boost/atomic.hpp>
#include <boost/atomic/atomic.hpp>
#include <chrono>
#include <cstdint>
#include <functional>
#include <iostream>
#include <memory>
#include <mutex>
#include <thread>
#include <type_traits>
#include <vector>

class join_threads {
  std::vector<std::thread> &threads;

public:
  explicit join_threads(std::vector<std::thread> &threads_)
      : threads(threads_) {}
  ~join_threads() {
    for (unsigned long i = 0; i < threads.size(); ++i) {
      if (threads[i].joinable()) {
        std::cout << "joined one\n";
        threads[i].join();
      }
    }
  }
};
class thread_pool {
  std::atomic_bool done;
  //join_threads joiner;
  threadsafe_queue<std::function<void()>> work_queue;
  std::vector<std::jthread> threads;
  void worker_thread() {
    while (!done) {
      std::function<void()> task;
      if (work_queue.try_pop(task))
        task();
      else
        std::this_thread::yield();
    }
  }

public:
  thread_pool() : done(false), work_queue(std::thread::hardware_concurrency()) {
    unsigned const thread_count = std::thread::hardware_concurrency();
    std::cout << thread_count << " threads available on this system\n";
    try {
      for (unsigned i = 0; i < thread_count; ++i) {
        break;
        threads.push_back(std::jthread(&thread_pool::worker_thread, this));
      }
    } catch (...) {
      done = true;
      throw;
    }
  }
  bool workdone(){return work_queue.isEmpty();}
  ~thread_pool() {
    while (work_queue.peek()) {
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    done = true;
  };
  template <typename FunctionType> void submit(FunctionType f) {
    work_queue.push(std::function<void()>(f));
  }
};
#endif

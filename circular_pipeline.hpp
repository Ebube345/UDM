/*
 * this class is going to contain the define the interface and the
 * implementation of our Circular buffer
 *
 * this class would either be or directly access our yet to be made allocator
 * class
 *
 * */
#ifndef CIRCULAR_PIPELINE_H
#define CIRCULAR_PIPELINE_H
#include <algorithm>
#include <memory>
#include <mutex>

template <typename T> class threadsafe_queue {
private:
  struct node {
    std::shared_ptr<T> data;
    std::unique_ptr<node> next;
  };
  int count = 0;
  std::mutex head_mutex;
  std::unique_ptr<node> head;
  std::mutex tail_mutex;
  node *tail;
  int size = 0;
  node *get_tail() {
    std::lock_guard<std::mutex> tail_lock(tail_mutex);
    return tail;
  }
  std::unique_ptr<node> pop_head() {
    std::lock_guard<std::mutex> head_lock(
        head_mutex); // double locks? is that safe?
    if (head.get() == get_tail()) {
      return nullptr;
    }
    std::unique_ptr<node> old_head = std::move(head);
    head = std::move(old_head->next);
    count--;
    return old_head;
  }
  std::unique_ptr<T> head_dataref() {
    std::lock_guard<std::mutex> head_lock(
        head_mutex); // double locks? is that safe?
    if (head.get() == get_tail()) {
      return nullptr;
    }
    std::unique_ptr<node> old_head = std::move(head);
    head = std::move(old_head->next);
    return old_head;
  }

public:
  threadsafe_queue(int siz) : head(new node), tail(head.get()), size(siz) {}
  threadsafe_queue(const threadsafe_queue &other) = delete;
  threadsafe_queue &operator=(const threadsafe_queue &other) = delete;
  threadsafe_queue &operator=(threadsafe_queue &&other) = delete;
  std::shared_ptr<T> try_pop() {
    std::unique_ptr<node> old_head = pop_head();
    return old_head ? old_head->data : std::shared_ptr<T>();
  }
  bool try_pop(T &value) {
    std::unique_ptr<node> old_head = pop_head();
    bool result = false;
    if (old_head) {
      // but what if the type has a deleted move constructor then we need to
      // have an overload for this function
      value = std::move(*old_head->data);
      result = true;
    } else {
      //value = std::move(value);
    }
    return result;
  }
  bool peek(void) {
    std::unique_ptr<node> old_head = pop_head();
    return old_head ? true : false;
  }
  bool isEmpty() {
    std::lock_guard<std::mutex> head_lock(
        head_mutex); // double locks? is that safe?
    if (head.get() == get_tail()) {
      return true;
    }
    return false;
  }
  T *pushWithRef(T new_value) {
    std::shared_ptr<T> new_data(std::make_shared<T>(std::move(new_value)));
    std::unique_ptr<node> p(new node);
    node *const new_tail = p.get();
    std::lock_guard<std::mutex> tail_lock(tail_mutex);
    tail->data = new_data;
    tail->next = std::move(p);
    T *tailRef = tail->data.get();
    tail = new_tail;
    count++;
    return tailRef;
  }
  void push(T new_value) {
    std::shared_ptr<T> new_data(std::make_shared<T>(std::move(new_value)));
    std::unique_ptr<node> p(std::make_unique<node>());
    node *const new_tail = p.get();
    std::lock_guard<std::mutex> tail_lock(tail_mutex);
    tail->data = new_data;
    tail->next = std::move(p);
    count++;
    tail = new_tail;
  }
  int getCount() { return count; }
};

#endif

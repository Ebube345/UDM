#include <cstdlib>
#include <iostream>
#include <memory>

class capsule;
template <typename T = capsule> class Queue {
  int size;
  int front;
  int back;
  struct node {
    std::shared_ptr<T> data;
    std::unique_ptr<node> next;
  };
  T *queueItems;

public:
  Queue(int sz = 128) {
    size = sz;
    front = -1;
    back = -1;
    queueItems = (T *)malloc(sizeof(T) * size);
  };
  virtual ~Queue() { delete[] queueItems; };
  bool isEmpty() {
    if (back == front)
      return true;
    return false;
  }
  bool isFull() {
    if ((back + 1) % size == front)
      return true;
    return false;
  }
  void push(T value) {
    // if (!isFull()) {
    back = (back + 1) % size;
    // fetch node at index back and insert
    queueItems[back] = value;
    //} else
    //// std::cout << "Queue is full\n";
    // insert fron the begining
  };
  T pop() {
    // if (!isEmpty()) {
    front = (front + 1) % size;
    // return the iten at index front
    return queueItems[front];
    //}
    // return NULL;
  };
  int getFront() { return front; }
  int getBack() { return back; }
};

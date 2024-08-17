#include "circularQueue.h"
#include <iostream>
#define SIZE 15
class A{
	public:
	int aa; 
	A() {aa = 4;}
};
A mya; 
int main(void) {
  Queue<int> myQueue(10);
  char arr[sizeof(Queue<int>) * mya.aa];
  std::cout << sizeof(arr) << std::endl;
  std::cout << sizeof(Queue<int>) << std::endl;
  for (int i = 0; i < SIZE; i++) {
    if (myQueue.isFull()) {
      std::cout << "My Queue is full\n";
      break;
    }
    myQueue.push(i + 1);
  }
  for (int i = 0; i < 10; i++) {
    std::cout << myQueue.pop() << std::endl;
  }
}

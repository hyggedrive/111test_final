#include "wrapper.h"
#include <iostream>
template <class T> void Wrapper<T>::print() {
  std::cout << x << std::endl;
}

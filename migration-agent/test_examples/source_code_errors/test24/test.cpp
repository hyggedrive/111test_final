#include <iostream>
#include <vector>
template <template <class> class Container, class T>
void printSize(const Container<T> &c) {
    std::cout << c.size() << std::endl;
}
int main() {
    std::vector<int> v(10);
    printSize(v);
    return 0;
}

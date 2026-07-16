#include <iostream>
#include "pi.h"
void func();
int main() {
    double d = pi::value<double>;
    std::cout << "main: " << d << std::endl;
    func();
    return 0;
}


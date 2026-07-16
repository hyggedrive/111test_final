#include <iostream>
#include "pi.h"
void func() {
    double d = pi::value<double>;
    std::cout << "func: " << d << std::endl;
}

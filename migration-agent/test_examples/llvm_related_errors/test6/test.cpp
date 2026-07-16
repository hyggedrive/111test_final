#include <cstdarg>
#include <iostream>

struct NonPod {
    NonPod() {}
    ~NonPod() {}
};

void sum(int count, ...) {
    va_list args;
    va_start(args, count);
    for (int i = 0; i < count; ++i) {
    NonPod obj = va_arg(args, NonPod);
    }
    va_end(args);
}

int main() {
    sum(2, 1, 2);
}

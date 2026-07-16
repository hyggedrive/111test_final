#include <iostream>
class CommonName {
    int x;
};
namespace ns::CommonName {
class C {
public:
    void print() {
        std::cout << "Hello, world" << std::endl;
    }
};
} // namespace ns::CommonName
using namespace ns;
    int main() {
    // ns::CommonName::C c;
    CommonName::C c;
    c.print();
    return 0;
}

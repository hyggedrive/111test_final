#include <iostream>

using namespace std;

class A
{
public:
    void A::printHello()
    {
        cout << "hello\n";
    }
};

int main()
{
    A().printHello();
    return 0;
}

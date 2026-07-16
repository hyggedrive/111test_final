#include <string>
#include <iostream>
template <typename T>
class Base {
public:
    void foo() {
        std::cout << "Base::foo()\n";
    }
};
template <typename T>
class Derived : public Base<T> {
	
public:
    void bar() {
	    foo();
    }
};

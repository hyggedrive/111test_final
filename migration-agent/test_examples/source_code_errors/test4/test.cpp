#include <iostream>
#include <memory> 
// #include <Poco/SharedPtr.h>

class MyClass {
public:
    MyClass() {
        std::cout << "MyClass constructor called" << std::endl;
    }
    ~MyClass() {
        std::cout << "MyClass destructor called" << std::endl;
    }
    void sayHello() const {
        std::cout << "Hello from MyClass" << std::endl;
    }
};

class MyClass2 {
public:
    MyClass2(Poco::SharedPtr<MyClass>& x) {
        std::cout << "MyClass2 constructor called" << std::endl;
        x->sayHello(); // 使用传递进来的 shared_ptr
    }
    ~MyClass2() {
        std::cout << "MyClass2 destructor called" << std::endl;
    }
};
template <typename T1>
int test() {
    MyClass2 x(new MyClass());
    return 0;    
}

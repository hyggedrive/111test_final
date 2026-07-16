#include <iostream>

class A {
public:
  virtual ~A() {std::cout << "executed ~A()\n";}
};

class B : public A {
public:
  virtual ~B() {std::cout << "executed ~B()\n";}
};

int main() {
    std::cout << "starting\n";
    B b;
    b.~A();    // 子类直接调用父类析构函数
    std::cout << "done\n";
}

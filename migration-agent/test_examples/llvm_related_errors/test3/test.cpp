struct A
{
    template<typename T> A(T) = delete;
};
template<> A::A<int>(int) {}  // error
// template<> A::A(int) {}  // no error
// A a(0);

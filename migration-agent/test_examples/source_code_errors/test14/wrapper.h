#ifndef WRAPPER_H
#define WRAPPER_H
template <class T> class Wrapper {
  private:
    T x;
  public:
    Wrapper() = default;
    Wrapper(const T &t) : x(t) {}
    void print();
  
};
template class Wrapper<int>;
#endif

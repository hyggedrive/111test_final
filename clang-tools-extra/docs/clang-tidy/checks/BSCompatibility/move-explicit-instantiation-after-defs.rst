.. title:: clang-tidy - BSCompatibility-move-explicit-instantiation-after-defs

BSCompatibility-move-explicit-instantiation-after-defs
======================================================

detect the explicit instantiation before defs and fix it.
e.g.
template <class T> class Wrapper {
  public:
    Wrapper() = default;
    Wrapper(const T &t) : x(t) {}
    void print();
  private:
    T x;
};
template class Wrapper<int>; // will be moved
template <class T> void Wrapper<T>::print() {
}
// will be move to here

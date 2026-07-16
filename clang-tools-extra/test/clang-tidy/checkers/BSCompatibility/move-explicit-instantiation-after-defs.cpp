// RUN: %check_clang_tidy %s BSCompatibility-move-explicit-instantiation-after-defs %t -- -fix-errors


// CHECK-MESSAGES: warning: explicit instantiation should appear after the out-of-line member definition(s) in this translation unit [BSCompatibility-move-explicit-instantiation-after-defs]

template <class T> class Wrapper {
  public:
    Wrapper() = default;
    Wrapper(const T &t) : x(t) {}
    void print();
    void print2();
    void print3();
  private:
    T x;
};

template <class T> void Wrapper<T>::print2()
{}

template class Wrapper<int>;
// CHECK-MESSAGES: note: suggest to remove this explicit instantiation here
template <class T> void Wrapper<T>::print() {
}

template <class T> void Wrapper<T>::print3(){
}

// CHECK-MESSAGES: note: suggest to insert the explicit instantiation here
// CHECK-FIXES: template class Wrapper<int>;

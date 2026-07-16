.. title:: clang-tidy - BSCompatibility-dependent-template-keyword

BSCompatibility-dependent-template-keyword
==========================================

When calling a dependent template function, keyword template is 
needed.   e.g.

class A {
public:
  template <typename> void As();
  static A *FromWebContents();
  A *FromWebContents2();
};
template <typename T> class B : A {
  void FromWebContents() {
    auto guest = A::FromWebContents();
    guest ? guest->As<T>() : nullptr;
    auto guest2 = A::FromWebContents2();
    guest2 ? guest2->As<T>() : nullptr;
  }
};

clang will report error unless adding a template before guest2,
while g++ dont have this issue.
this check detect every ways to call function,through .,->,:: and will
add template before them.
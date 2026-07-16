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
}

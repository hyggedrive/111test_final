// RUN: %check_clang_tidy %s BSCompatibility-dependent-template-keyword %t -- -fix-errors


class A {
public:
  template <typename> void As();
  void As();
  static A *FromWebContents();
  A *FromWebContents2();
};

template <typename T> class AA {
public:
  template <typename> void As();
  void As();
};

class AAA {
public:
  template <typename> static void As();
  void As();
};

template <typename T> void Aas();

template <typename U> class B : A {
  void foo() {
    AA<U> *aa = new AA<U>;

    aa->As<int>();
    // CHECK-MESSAGE: :[[@LINE-1]]:9: error: use 'template' keyword to treat 'As' as a dependent template name [clang-diagnostic-error]
    // CHECK-FIXES: aa->template As<int>();
    aa->As<U>();
    // CHECK-MESSAGE: :[[@LINE-1]]:9: error: use 'template' keyword to treat 'As' as a dependent template name [clang-diagnostic-error]
    // CHECK-FIXES: (*aa).template As<U>();
    aa->template As<U>();
    (*aa).As();
    (*aa).As<U>();
    // CHECK-MESSAGE: :[[@LINE-1]]:11: error: use 'template' keyword to treat 'As' as a dependent template name [clang-diagnostic-error]
    // CHECK-FIXES: (*aa).template As<U>();
    (*aa).template As<U>();
    auto guest = A::FromWebContents();
    guest->As<U>();
    guest->template As<U>();
    auto guest2 = A::FromWebContents2();
    guest2->template As<U>();
    guest2->As<U>();
    // CHECK-MESSAGE: :[[@LINE-1]]:13: error: use 'template' keyword to treat 'As' as a dependent template name [clang-diagnostic-error]
    // CHECK-FIXES: guest2->template As<U>();
  }
};
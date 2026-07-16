#include <array>
#include <iostream>
struct S {
  static constexpr int Dim = 3;
};
void func(const S &s) {
  std::array<int, s.Dim> a;
  std::cout << s.Dim << std::endl;
  std::cout << a.size() << std::endl;
}
int main() {
  S s;
  func(s);
  return 0;
}

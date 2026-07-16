// RUN: %check_clang_tidy %s BSCompatibility-redundant-default-template-arg %t -- -fix-errors
// the first declearation will be remained
template <typename T = int> void printSize(void);

template <typename T = double>
// CHECK-MESSAGES: :[[@LINE-1]]:20: warning: default template argument redefined; only the first declaration should specify it [BSCompatibility-redundant-default-template-arg]
// CHECK-MESSAGES: :[[@LINE-2]]:24: error: template parameter redefines default argument [clang-diagnostic-error]
//CHECK-FIXES: template <typename T>
void printSize();

template <typename T = double>
// CHECK-MESSAGES: :[[@LINE-1]]:20: warning: default template argument redefined; only the first declaration should specify it [BSCompatibility-redundant-default-template-arg]
// CHECK-MESSAGES: :[[@LINE-2]]:24: error: template parameter redefines default argument [clang-diagnostic-error]
//CHECK-FIXES: template <typename T>
void printSize() {
  T a = 3.15;
}

template <typename T = char> int retSize(T x);

template <typename T = double> int retSize(T x);
// CHECK-MESSAGES: :[[@LINE-1]]:20: warning: default template argument redefined; only the first declaration should specify it [BSCompatibility-redundant-default-template-arg]
// CHECK-MESSAGES: :[[@LINE-2]]:24: error: template parameter redefines default argument [clang-diagnostic-error]
// CHECK-FIXES: template <typename T> int retSize(T x);

template <typename T = int>
// CHECK-MESSAGES: :[[@LINE-1]]:20: warning: default template argument redefined; only the first declaration should specify it [BSCompatibility-redundant-default-template-arg]
// CHECK-MESSAGES: :[[@LINE-2]]:24: error: template parameter redefines default argument [clang-diagnostic-error]
// CHECK-FIXES: template <typename T>
int retSize(T x)
{

}

int main() {
  printSize<>();
  return 0;
}
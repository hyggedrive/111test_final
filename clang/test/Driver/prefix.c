// RUN: %clang %s
// RUN: %clang -iprefix %resource_dir/../ -iwithprefix include %s
// RUN: %clang -iwithprefix include %s
// RUN: %clang -nostdinc -iwithprefix include %s -fgcc-compatible
// RUN: %clang -nostdinc -iprefix %resource_dir/../ -iwithprefix include %s -fgcc-compatible
// RUN: not %clang -nostdinc -iwithprefix include %s
// RUN: not %clang -nostdinc -iprefix "" -iwithprefix include %s

#include <stdarg.h>
int main(void) { return 0; }

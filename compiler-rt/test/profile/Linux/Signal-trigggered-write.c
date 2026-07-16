// REQUIRES: linux
 
// Comment: this is the expected normal situation where the exit status code is
// not 0 due to the signal sent by the application being unhandled.
// RUN: rm -f %t.profraw1
// RUN: %clang_profgen -o %t %s
// RUN: env LLVM_PROFILE_FILE=%t.profraw1 PGODUMPSIGNAL="0" %run %t || true
// RUN: cat %t.profraw1 | FileCheck --check-prefix=CHECK-OFF --allow-empty %s
 
// RUN: rm -f %t.profraw2
// RUN: %clang_profgen -o %t %s -DUSE_SIGUSR1
// RUN: env LLVM_PROFILE_FILE=%t.profraw2 PGODUMPSIGNAL=`kill -l SIGUSR1` %run %t
// RUN: llvm-profdata show %t.profraw2 --all-functions | FileCheck %s
 
// RUN: rm -f %t.profraw3
// RUN: %clang_profgen -o %t %s
// RUN: env LLVM_PROFILE_FILE=%t.profraw3 PGODUMPSIGNAL=`kill -l SIGUSR2` %run %t
// RUN: llvm-profdata show %t.profraw3 --all-functions | FileCheck %s
 
// CHECK: main:
// CHECK-OFF-NOT: .
#include <signal.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>

int printf(const char *format, ...);
int main(int argc, char *argv[])
{
#if defined(USE_SIGUSR1)
    printf("use signal 1");
    kill(getpid(), SIGUSR1);
#else
    printf("use signal 2");
    kill(getpid(), SIGUSR2);
#endif
    _exit(0);
}
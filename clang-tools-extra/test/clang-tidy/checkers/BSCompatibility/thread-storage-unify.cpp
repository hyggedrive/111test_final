// RUN: %check_clang_tidy %s BSCompatibility-thread-storage-unify %t -- -fix-errors

extern thread_local long test;
// CHECK-MESSAGES: warning: unify to GNU '__thread' to match redeclarations [BSCompatibility-thread-storage-unify]
// CHECK-MESSAGES: warning: mixed use of '__thread' and 'thread_local' for the same variable; unifying to GNU '__thread' [BSCompatibility-thread-storage-unify]
// CHECK-FIXES: extern __thread long test;
extern __thread long test;
// CHECK-MESSAGES: error: thread-local declaration of 'test' with static initialization follows declaration with dynamic initialization [clang-diagnostic-error]


extern __thread int b;
// CHECK-MESSAGES: warning: unify to 'thread_local' to match the last extern declaration [BSCompatibility-thread-storage-unify]
// CHECK-MESSAGES: warning: mixed use of '__thread' and 'thread_local' for the same variable; unifying to 'thread_local' per the last extern declaration [BSCompatibility-thread-storage-unify]
// CHECK-FIXES: extern thread_local int b;
extern thread_local int b;
// CHECK-MESSAGES: error: thread-local declaration of 'b' with dynamic initialization follows declaration with static initialization [clang-diagnostic-error]


extern __thread int c;
// CHECK-MESSAGES: warning: mixed use of '__thread' and 'thread_local' for the same variable; unifying to GNU '__thread' [BSCompatibility-thread-storage-unify]

thread_local int c;
// CHECK-MESSAGES: warning: unify to GNU '__thread' for non-extern or defined thread-local variable [BSCompatibility-thread-storage-unify]
// CHECK-MESSAGES: error: thread-local declaration of 'c' with dynamic initialization follows declaration with static initialization [clang-diagnostic-error]
// CHECK_FIXES: __thread int c;

thread_local int d;
// CHECK-MESSAGES: warning: unify to GNU '__thread' for non-extern or defined thread-local variable [BSCompatibility-thread-storage-unify]
// CHECK-FIXES: __thread int d;
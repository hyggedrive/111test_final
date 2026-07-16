.. title:: clang-tidy - BSCompatibility-thread-storage-unify

BSCompatibility-thread-storage-unify
====================================

detect the mixed use of __thread and thread_local and fix it.
e.g.
extern thread_local long test;// FIXES: extern __thread long test;

__thread long test;

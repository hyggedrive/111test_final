#ifndef __PRAGMA_REDEFINE_EXTNAME
#error
#endif
namespace somewhere {
    extern "C" int whiz(void);
    int whiz(int);
}
#pragma redefine_extname whiz bang
int (*s)() = somewhere::whiz;
namespace elsewhere {
    extern "C" int whiz(void);
}
int (*t)() = elsewhere::whiz;

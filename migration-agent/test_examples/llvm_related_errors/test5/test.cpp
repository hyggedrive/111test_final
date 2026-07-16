#include <stdlib.h>
#include <stdio.h>
int test, failed;
int main (void);
void eh1 (void *p, int x)
{
    printf("eh1\n");
    void *q = __builtin_alloca (x);
    __builtin_eh_return (0, p);
}
void fail (void)
{
    printf ("failed\n");
    abort ();
}
void continuation (void)
{
    printf ("continuation\n");
    test++;
    main();
}
int main (void)
{
    printf("main\n");
    if(test == 0) eh1 (continuation, 100);
    printf("exit\n");
    exit (0);
}

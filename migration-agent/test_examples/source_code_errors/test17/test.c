int foo (double a, double b)
{
    double square (double z){ return z * z; }
    return square (a) + square (b);
}

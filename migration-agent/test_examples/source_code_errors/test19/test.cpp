template <class T>
void f (T* p)
{
  p->~auto();
}

int d;
struct A { ~A() { ++d; } };

int main()
{
  f(new int(42));
  f(new A);
  if (d != 1)
    throw;
  return 0;
}

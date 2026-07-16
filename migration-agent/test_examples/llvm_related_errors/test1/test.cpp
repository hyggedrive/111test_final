template <typename T>
class C
{
public:
    void mf1() const {}
    void mf2() const {}

private:
    T t;
};
template void C<double>::mf1() const;
template class C<double>;
.. title:: clang-tidy - BSCompatibility-redundant-default-template-arg

BSCompatibility-redundant-default-template-arg
==============================================

when there are redundant default template arguments for template function,
clang will raise an error while gcc will accept it, using the first default arg.
This check detect the issue and will fix it by gcc rules.

e.g.
template <typename T = int> void printSize(void);

template <typename T = double>
void printSize();

delete the "= double"
#include "explicit-function-template-instantiation.h"

template <typename T> T identity(const T &value) { return value; }
template <int N> int constant() { return N; }
template <int N> int oops() { return N; }

template int identity<int>(const int &);
template double identity<double>(const double &);

template int constant<42>();

template int oops<123>();
template int oops<321>();
template int oops<42>();

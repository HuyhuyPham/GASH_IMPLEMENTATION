#ifndef GASH_INSPECT_H
#define GASH_INSPECT_H

#include "List.h"

#include <iostream>
#include <ostream>
#include <string>

namespace gash {

// Print a list without changing it. Supported element types are explicitly
// instantiated in Inspect.cpp.
template <typename T>
void inspect(const List<T>& list, std::ostream& out = std::cout);

// Explicit instantiation declarations for the GASH primitive element types.
extern template void inspect<int>(const List<int>&, std::ostream&);
extern template void inspect<double>(const List<double>&, std::ostream&);
extern template void inspect<bool>(const List<bool>&, std::ostream&);
extern template void inspect<char>(const List<char>&, std::ostream&);
extern template void inspect<std::string>(const List<std::string>&, std::ostream&);

} // namespace gash

#endif // GASH_INSPECT_H

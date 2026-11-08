
#include "Inspect.h"

#include <cstdint>
#include <iomanip>
#include <type_traits>

namespace gash {

namespace detail {

template <typename T>
constexpr const char* listElementTypeName() {
    if constexpr (std::is_same_v<T, int>)
        return "int";
    else if constexpr (std::is_same_v<T, double>)
        return "float";
    else if constexpr (std::is_same_v<T, bool>)
        return "bool";
    else if constexpr (std::is_same_v<T, char>)
        return "char";
    else if constexpr (std::is_same_v<T, std::string>)
        return "string";
    else
        return "unknown";
}

template <typename T>
void printListValue(std::ostream& out, const T& value) {
    if constexpr (std::is_same_v<T, std::string>) {
        out << std::quoted(value);
    }
    else if constexpr (std::is_same_v<T, char>) {
        out << '\'' << value << '\'';
    }
    else if constexpr (std::is_same_v<T, bool>) {
        out << (value ? "true" : "false");
    }
    else {
        out << value;
    }
}

} // namespace detail

template <typename T>
void inspect(const List<T>& list, std::ostream& out) {
    out << "List<"
        << detail::listElementTypeName<T>()
        << "> size=" << list.size()
        << " capacity=" << list.capacity()
        << '\n';

    for (std::size_t i = 0; i < list.size(); ++i) {
        out << "  [" << i << "] ";

        const auto index = static_cast<std::int64_t>(i);

        if (list.isInitialized(index)) {
            detail::printListValue(
                out, list.get(index)
            );
        }
        else {
            out << "<uninitialized>";
        }

        out << '\n';
    }
}

// Explicit template instantiations
template void inspect<int>(
    const List<int>&, std::ostream&);

template void inspect<double>(
    const List<double>&, std::ostream&);

template void inspect<bool>(
    const List<bool>&, std::ostream&);

template void inspect<char>(
    const List<char>&, std::ostream&);

template void inspect<std::string>(
    const List<std::string>&, std::ostream&);

} // namespace gash

// Implements non-template List validation helpers and instantiates List<T> for primitive types.
#include "List.h"

#include <stdexcept>
#include <string>

namespace gash {

namespace detail {

// Verifies that `index` lies within [0, size) and converts it to std::size_t.
std::size_t checkedListIndex(std::int64_t index, std::size_t size) {
    if (index < 0 || static_cast<std::uint64_t>(index) >= size) {
        throw std::out_of_range(
            "List index out of bounds: " + std::to_string(index));
    }
    return static_cast<std::size_t>(index);
}

// Validates a non-negative list resize target against container maximum size.
std::size_t validateListResize(std::int64_t newSize, std::size_t maxSize) {
    if (newSize < 0) {
        throw std::length_error("List size cannot be negative");
    }
    const auto target = static_cast<std::uint64_t>(newSize);
    if (target > maxSize) {
        throw std::length_error("List size exceeds supported maximum");
    }
    return static_cast<std::size_t>(target);
}

// Throws a runtime error when reading an uninitialized list position.
[[noreturn]] void throwUninitializedListRead(std::int64_t index) {
    throw std::runtime_error(
        "Read of uninitialized list element at index " + std::to_string(index));
}

} // namespace detail

// Explicit template instantiations for GASH's supported primitive list types.
template class List<int>;
template class List<double>;
template class List<bool>;
template class List<char>;
template class List<std::string>;

} // namespace gash

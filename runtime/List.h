// Declares and implements the bounds-checked, occupancy-tracked List<T> template container.
#ifndef GASH_LIST_H
#define GASH_LIST_H

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace gash {

namespace detail {

// Non-template bounds/length validation and error reporting helpers (defined in List.cpp).
std::size_t checkedListIndex(std::int64_t index, std::size_t size);
std::size_t validateListResize(std::int64_t newSize, std::size_t maxSize);
[[noreturn]] void throwUninitializedListRead(std::int64_t index);

} // namespace detail

// A typed list with independently tracked, initially uninitialized slots.
// Guard/lock validation is the responsibility of the agreed runtime mutation layer.
template <typename T>
class List {
public:
    List() = default;

    explicit List(std::int64_t length) {
        resize(length);
    }

    // std::vector<std::optional<T>> owns each element and gives value-copy semantics.
    List(const List&) = default;
    List& operator=(const List&) = default;
    List(List&&) = default;
    List& operator=(List&&) = default;
    ~List() = default;

    // Returns the number of addressable positions in the list.
    std::size_t size() const noexcept { return slots_.size(); }
    // Returns the current allocated storage capacity of the list.
    std::size_t capacity() const noexcept { return slots_.capacity(); }

    // Returns whether the slot at `index` currently holds a value.
    bool isInitialized(std::int64_t index) const {
        return slots_[detail::checkedListIndex(index, slots_.size())].has_value();
    }

    // Reads the initialized element at `index`, throwing if out-of-bounds or uninitialized.
    const T& get(std::int64_t index) const {
        const auto& slot = slots_[detail::checkedListIndex(index, slots_.size())];
        if (!slot.has_value()) {
            detail::throwUninitializedListRead(index);
        }
        return *slot;
    }

    // Assigns `value` at `index` (only call after any Environment lock/guard checks pass).
    void set(std::int64_t index, const T& value) {
        slots_[detail::checkedListIndex(index, slots_.size())] = value;
    }

    // Resizes the logical length to `newSize`, discarding truncated items or adding empty slots.
    void resize(std::int64_t newSize) {
        const std::size_t target = detail::validateListResize(newSize, slots_.max_size());
        // Growing introduces empty optionals. Shrinking destroys truncated slots.
        // std::vector::resize does not shrink allocated capacity.
        slots_.resize(target);
    }

private:
    std::vector<std::optional<T>> slots_;
};

// Explicit instantiation declarations for the five GASH primitive element types
// (instantiated once in runtime/List.cpp to minimize compile times).
extern template class List<int>;
extern template class List<double>;
extern template class List<bool>;
extern template class List<char>;
extern template class List<std::string>;

} // namespace gash

#endif // GASH_LIST_H

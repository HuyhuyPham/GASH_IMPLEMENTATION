#ifndef GASH_LIST_H
#define GASH_LIST_H

#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace gash {

// A typed list with independently tracked, initially uninitialized slots.
// Guard/lock validation is the responsibility of the agreed runtime mutation layer.
template <typename T>
class List {
public:
    explicit List(std::int64_t length) {
        resize(length);
    }

    // std::vector<std::optional<T>> owns each element and gives value-copy semantics.
    List(const List&) = default;
    List& operator=(const List&) = default;
    List(List&&) = default;
    List& operator=(List&&) = default;
    ~List() = default;

    std::size_t size() const noexcept { return slots_.size(); }
    std::size_t capacity() const noexcept { return slots_.capacity(); }

    bool isInitialized(std::int64_t index) const {
        return slots_[checkedIndex(index)].has_value();
    }

    const T& get(std::int64_t index) const {
        const auto& slot = slots_[checkedIndex(index)];
        if (!slot.has_value()) {
            throw std::runtime_error("Read of uninitialized list element at index " +
                                     std::to_string(index));
        }
        return *slot;
    }

    // Only call after any Environment lock/guard checks have passed.
    void set(std::int64_t index, const T& value) {
        slots_[checkedIndex(index)] = value;
    }

    void resize(std::int64_t newSize) {
        if (newSize < 0) {
            throw std::length_error("List size cannot be negative");
        }
        const auto target = static_cast<std::uint64_t>(newSize);
        if (target > slots_.max_size()) {
            throw std::length_error("List size exceeds supported maximum");
        }
        // Growing introduces empty optionals. Shrinking destroys truncated slots.
        // std::vector::resize does not shrink allocated capacity.
        slots_.resize(static_cast<std::size_t>(target));
    }

private:
    std::vector<std::optional<T>> slots_;

    std::size_t checkedIndex(std::int64_t index) const {
        if (index < 0 || static_cast<std::uint64_t>(index) >= slots_.size()) {
            throw std::out_of_range("List index out of bounds: " +
                                    std::to_string(index));
        }
        return static_cast<std::size_t>(index);
    }
};

} // namespace gash

#endif // GASH_LIST_H

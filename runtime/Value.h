// Declares ListValue, Value, explicit conversion helpers, and checked arithmetic templates.
#ifndef GASH_VALUE_H
#define GASH_VALUE_H

#if __has_include("../include/Type.h")
#include "../include/Type.h"
#else
#include "Type.h"
#endif

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <limits>
#include <optional>
#include <ostream>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <variant>
#include <vector>

namespace gash {

class Value;

// Represents a typed GASH runtime list with value-copy (deep-copy) semantics.
class ListValue {
public:
    using ElementVariant = std::variant<int, double, bool, char, std::string>;

    ListValue();
    ListValue(Type elementType, std::int64_t initialSize, std::size_t capacityHint = 0);

    ListValue(const ListValue&) = default;
    ListValue& operator=(const ListValue&) = default;
    ListValue(ListValue&&) noexcept = default;
    ListValue& operator=(ListValue&&) noexcept = default;
    ~ListValue() = default;

    Type elementType() const noexcept;
    Type type() const;

    std::size_t size() const noexcept;
    std::size_t capacity() const noexcept;
    std::size_t declaredCapacity() const noexcept;

    void setDeclaredCapacity(std::size_t cap) noexcept;

    bool isInitialized(std::int64_t index) const;
    Value get(std::int64_t index) const;
    void set(std::int64_t index, const Value& value);
    void resize(std::int64_t newSize);

    bool operator==(const ListValue& other) const;
    bool operator!=(const ListValue& other) const;

private:
    Type elementType_;
    std::vector<std::optional<ElementVariant>> slots_;
    std::size_t declaredCapacity_ = 0;

    std::size_t checkedIndex(std::int64_t index) const;
};

// Represents any runtime value in the GASH environment.
// Uses value semantics so copying a Value (including lists) performs a deep copy.
class Value {
public:
    using Storage = std::variant<
        std::monostate,
        int,
        double,
        bool,
        char,
        std::string,
        ListValue
    >;

    // Default constructs an uninitialized value.
    Value() = default;

    Value(int v);
    Value(double v);
    Value(bool v);
    Value(char v);
    Value(std::string v);
    Value(const char* v);
    Value(ListValue v);

    static Value Int(int v);
    static Value Float(double v);
    static Value Bool(bool v);
    static Value Char(char v);
    static Value String(std::string v);
    static Value List(Type elementType, std::int64_t size, std::size_t capacity = 0);

    bool isInitialized() const noexcept;
    bool isInt() const noexcept;
    bool isFloat() const noexcept;
    bool isBool() const noexcept;
    bool isChar() const noexcept;
    bool isString() const noexcept;
    bool isList() const noexcept;

    Type type() const;

    int asInt() const;
    double asFloat() const;
    bool asBool() const;
    char asChar() const;
    const std::string& asString() const;
    const ListValue& asList() const;
    ListValue& asList();

    // Explicit type conversion helpers: int(x), float(x), string(x)
    Value castToInt() const;
    Value castToFloat() const;
    Value castToString() const;

    std::string toString() const;

    bool operator==(const Value& other) const;
    bool operator!=(const Value& other) const;

    // Binding identity metadata used by Checkpoint/Restore (Assumptions A38, A41).
    std::size_t bindingId() const noexcept;
    void setBindingId(std::size_t id) noexcept;

private:
    Storage data_;
    std::size_t bindingId_ = 0;
};

// Emitted runtime conversion and I/O helpers (Assumptions A31, S4, H7).
std::string input(std::istream& in = std::cin);

int toInt(int v);
int toInt(double v);
int toInt(bool v);
int toInt(char v);
int toInt(const std::string& v);
int toInt(const char* v);
int toInt(const Value& v);

double toFloat(int v);
double toFloat(double v);
double toFloat(bool v);
double toFloat(char v);
double toFloat(const std::string& v);
double toFloat(const char* v);
double toFloat(const Value& v);

std::string toString(int v);
std::string toString(double v);
std::string toString(bool v);
std::string toString(char v);
std::string toString(const std::string& v);
std::string toString(const char* v);
std::string toString(const Value& v);

namespace detail {

[[noreturn]] void throwDivisionByZero();
[[noreturn]] void throwDivisionOverflow();
[[noreturn]] void throwInvalidExponentiation();

} // namespace detail

int safeModulo(int lhs, int rhs);

// Checked arithmetic templates ensuring invalid operations fail cleanly at runtime.
template <typename T>
inline T safeDivide(T lhs, T rhs) {
    if (rhs == static_cast<T>(0)) {
        detail::throwDivisionByZero();
    }
    if constexpr (std::is_integral_v<T> && std::is_signed_v<T>) {
        if (lhs == std::numeric_limits<T>::min() && rhs == static_cast<T>(-1)) {
            detail::throwDivisionOverflow();
        }
    }
    return lhs / rhs;
}

template <typename T>
inline T safePower(T lhs, T rhs) {
    double result = std::pow(static_cast<double>(lhs), static_cast<double>(rhs));
    if (!std::isfinite(result)) {
        detail::throwInvalidExponentiation();
    }
    return static_cast<T>(result);
}

// Uniform formatted output helper matching GASH bool/list display conventions.
template <typename T>
inline void writeOutput(std::ostream& out, const T& value) {
    if constexpr (std::is_same_v<std::decay_t<T>, bool>) {
        out << (value ? "true" : "false") << '\n';
    } else if constexpr (std::is_same_v<std::decay_t<T>, Value>) {
        out << value.toString() << '\n';
    } else {
        out << value << '\n';
    }
}

extern template int safeDivide<int>(int, int);
extern template double safeDivide<double>(double, double);
extern template int safePower<int>(int, int);
extern template double safePower<double>(double, double);
extern template void writeOutput<int>(std::ostream&, const int&);
extern template void writeOutput<double>(std::ostream&, const double&);
extern template void writeOutput<bool>(std::ostream&, const bool&);
extern template void writeOutput<char>(std::ostream&, const char&);
extern template void writeOutput<std::string>(std::ostream&, const std::string&);
extern template void writeOutput<Value>(std::ostream&, const Value&);

} // namespace gash

#endif // GASH_VALUE_H

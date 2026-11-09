#ifndef GASH_VALUE_H
#define GASH_VALUE_H

#include "Type.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <variant>
#include <vector>

namespace gash {

class Value;

// Represents a typed GASH runtime list with value-copy (deep-copy) semantics.
class ListValue {
public:
    using ElementVariant = std::variant<int, double, bool, char, std::string>;

    ListValue() : elementType_(Type::Int()), declaredCapacity_(0) {}

    ListValue(Type elementType, std::int64_t initialSize, std::size_t capacityHint = 0)
        : elementType_(elementType), declaredCapacity_(0) {
        if (!elementType_.isPrimitive()) {
            throw std::invalid_argument(
                "List element type must be a primitive type, got '" +
                elementType_.toString() + "'");
        }
        resize(initialSize);
        if (capacityHint > declaredCapacity_) {
            declaredCapacity_ = capacityHint;
        }
    }

    ListValue(const ListValue&) = default;
    ListValue& operator=(const ListValue&) = default;
    ListValue(ListValue&&) noexcept = default;
    ListValue& operator=(ListValue&&) noexcept = default;
    ~ListValue() = default;

    Type elementType() const noexcept { return elementType_; }
    Type type() const { return Type::ListOf(elementType_); }

    std::size_t size() const noexcept { return slots_.size(); }
    std::size_t capacity() const noexcept {
        return std::max(slots_.capacity(), declaredCapacity_);
    }
    std::size_t declaredCapacity() const noexcept { return declaredCapacity_; }

    void setDeclaredCapacity(std::size_t cap) noexcept {
        declaredCapacity_ = std::max(cap, slots_.size());
    }

    bool isInitialized(std::int64_t index) const {
        return slots_[checkedIndex(index)].has_value();
    }

    Value get(std::int64_t index) const;

    void set(std::int64_t index, const Value& value);

    void resize(std::int64_t newSize) {
        if (newSize < 0) {
            throw std::runtime_error("List size cannot be negative");
        }
        const auto target = static_cast<std::size_t>(newSize);
        slots_.resize(target);
        if (target > declaredCapacity_) {
            declaredCapacity_ = target;
        }
    }

    bool operator==(const ListValue& other) const {
        return elementType_ == other.elementType_ &&
               declaredCapacity_ == other.declaredCapacity_ &&
               slots_ == other.slots_;
    }

    bool operator!=(const ListValue& other) const {
        return !(*this == other);
    }

private:
    Type elementType_;
    std::vector<std::optional<ElementVariant>> slots_;
    std::size_t declaredCapacity_ = 0;

    std::size_t checkedIndex(std::int64_t index) const {
        if (index < 0 || static_cast<std::size_t>(index) >= slots_.size()) {
            throw std::out_of_range(
                "List index out of bounds: " + std::to_string(index));
        }
        return static_cast<std::size_t>(index);
    }
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

    Value(int v) : data_(v) {}
    Value(double v) : data_(v) {}
    Value(bool v) : data_(v) {}
    Value(char v) : data_(v) {}
    Value(std::string v) : data_(std::move(v)) {}
    Value(const char* v) : data_(std::string(v ? v : "")) {}
    Value(ListValue v) : data_(std::move(v)) {}

    static Value Int(int v) { return Value(v); }
    static Value Float(double v) { return Value(v); }
    static Value Bool(bool v) { return Value(v); }
    static Value Char(char v) { return Value(v); }
    static Value String(std::string v) { return Value(std::move(v)); }
    static Value List(Type elementType, std::int64_t size, std::size_t capacity = 0) {
        return Value(ListValue(elementType, size, capacity));
    }

    bool isInitialized() const noexcept {
        return !std::holds_alternative<std::monostate>(data_);
    }

    bool isInt() const noexcept { return std::holds_alternative<int>(data_); }
    bool isFloat() const noexcept { return std::holds_alternative<double>(data_); }
    bool isBool() const noexcept { return std::holds_alternative<bool>(data_); }
    bool isChar() const noexcept { return std::holds_alternative<char>(data_); }
    bool isString() const noexcept { return std::holds_alternative<std::string>(data_); }
    bool isList() const noexcept { return std::holds_alternative<ListValue>(data_); }

    Type type() const {
        if (std::holds_alternative<int>(data_)) return Type::Int();
        if (std::holds_alternative<double>(data_)) return Type::Float();
        if (std::holds_alternative<bool>(data_)) return Type::Bool();
        if (std::holds_alternative<char>(data_)) return Type::Char();
        if (std::holds_alternative<std::string>(data_)) return Type::String();
        if (const auto* list = std::get_if<ListValue>(&data_)) {
            return list->type();
        }
        return Type::Unknown();
    }

    int asInt() const {
        if (const auto* p = std::get_if<int>(&data_)) {
            return *p;
        }
        throw std::runtime_error(
            "Expected 'int' value, got '" + type().toString() + "'");
    }

    double asFloat() const {
        if (const auto* p = std::get_if<double>(&data_)) {
            return *p;
        }
        throw std::runtime_error(
            "Expected 'float' value, got '" + type().toString() + "'");
    }

    bool asBool() const {
        if (const auto* p = std::get_if<bool>(&data_)) {
            return *p;
        }
        throw std::runtime_error(
            "Expected 'bool' value, got '" + type().toString() + "'");
    }

    char asChar() const {
        if (const auto* p = std::get_if<char>(&data_)) {
            return *p;
        }
        throw std::runtime_error(
            "Expected 'char' value, got '" + type().toString() + "'");
    }

    const std::string& asString() const {
        if (const auto* p = std::get_if<std::string>(&data_)) {
            return *p;
        }
        throw std::runtime_error(
            "Expected 'string' value, got '" + type().toString() + "'");
    }

    const ListValue& asList() const {
        if (const auto* p = std::get_if<ListValue>(&data_)) {
            return *p;
        }
        throw std::runtime_error(
            "Expected 'list' value, got '" + type().toString() + "'");
    }

    ListValue& asList() {
        if (auto* p = std::get_if<ListValue>(&data_)) {
            return *p;
        }
        throw std::runtime_error(
            "Expected 'list' value, got '" + type().toString() + "'");
    }

    // Explicit type conversion helpers: int(x), float(x), string(x)
    Value castToInt() const {
        if (isInt()) return *this;
        if (isFloat()) return Value(static_cast<int>(asFloat()));
        if (isBool()) return Value(asBool() ? 1 : 0);
        if (isChar()) return Value(static_cast<int>(asChar()));
        if (isString()) {
            const std::string& s = asString();
            std::size_t pos = 0;
            int parsed = 0;
            try {
                parsed = std::stoi(s, &pos);
            } catch (const std::exception&) {
                throw std::runtime_error("Cannot convert string '" + s + "' to int");
            }
            if (pos != s.size()) {
                throw std::runtime_error("Cannot convert string '" + s + "' to int");
            }
            return Value(parsed);
        }
        throw std::runtime_error("Cannot convert '" + type().toString() + "' to int");
    }

    Value castToFloat() const {
        if (isFloat()) return *this;
        if (isInt()) return Value(static_cast<double>(asInt()));
        if (isBool()) return Value(asBool() ? 1.0 : 0.0);
        if (isChar()) return Value(static_cast<double>(asChar()));
        if (isString()) {
            const std::string& s = asString();
            std::size_t pos = 0;
            double parsed = 0.0;
            try {
                parsed = std::stod(s, &pos);
            } catch (const std::exception&) {
                throw std::runtime_error("Cannot convert string '" + s + "' to float");
            }
            if (pos != s.size()) {
                throw std::runtime_error("Cannot convert string '" + s + "' to float");
            }
            return Value(parsed);
        }
        throw std::runtime_error("Cannot convert '" + type().toString() + "' to float");
    }

    Value castToString() const {
        if (isString()) return *this;
        if (isInt()) return Value(std::to_string(asInt()));
        if (isFloat()) {
            std::ostringstream oss;
            oss << asFloat();
            return Value(oss.str());
        }
        if (isBool()) return Value(std::string(asBool() ? "true" : "false"));
        if (isChar()) return Value(std::string(1, asChar()));
        throw std::runtime_error("Cannot convert '" + type().toString() + "' to string");
    }

    std::string toString() const {
        if (!isInitialized()) return "<uninitialized>";
        if (isInt()) return std::to_string(asInt());
        if (isFloat()) {
            std::ostringstream oss;
            oss << asFloat();
            return oss.str();
        }
        if (isBool()) return asBool() ? "true" : "false";
        if (isChar()) return std::string(1, asChar());
        if (isString()) return asString();
        if (isList()) {
            const auto& lst = asList();
            return lst.type().toString() + "(size=" + std::to_string(lst.size()) + ")";
        }
        return "<unknown>";
    }

    bool operator==(const Value& other) const {
        return data_ == other.data_;
    }

    bool operator!=(const Value& other) const {
        return !(*this == other);
    }

    // Binding identity metadata used by Checkpoint/Restore (Assumptions A38, A41).
    std::size_t bindingId() const noexcept { return bindingId_; }
    void setBindingId(std::size_t id) noexcept { bindingId_ = id; }

private:
    Storage data_;
    std::size_t bindingId_ = 0;
};

inline Value ListValue::get(std::int64_t index) const {
    const auto& slot = slots_[checkedIndex(index)];
    if (!slot.has_value()) {
        throw std::runtime_error(
            "Read of uninitialized list element at index " + std::to_string(index));
    }
    return std::visit([](const auto& elem) { return Value(elem); }, *slot);
}

inline void ListValue::set(std::int64_t index, const Value& value) {
    if (!value.isInitialized()) {
        throw std::runtime_error("Cannot assign uninitialized value to list element");
    }
    if (value.type() != elementType_) {
        throw std::runtime_error(
            "List element type mismatch: expected '" + elementType_.toString() +
            "', got '" + value.type().toString() + "'");
    }
    const std::size_t idx = checkedIndex(index);
    switch (elementType_.kind) {
        case TypeKind::Int:
            slots_[idx] = value.asInt();
            break;
        case TypeKind::Float:
            slots_[idx] = value.asFloat();
            break;
        case TypeKind::Bool:
            slots_[idx] = value.asBool();
            break;
        case TypeKind::Char:
            slots_[idx] = value.asChar();
            break;
        case TypeKind::String:
            slots_[idx] = value.asString();
            break;
        default:
            throw std::runtime_error("Unsupported list element type");
    }
}

} // namespace gash

#endif // GASH_VALUE_H

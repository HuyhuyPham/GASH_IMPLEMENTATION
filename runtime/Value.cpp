// Implements ListValue, Value, runtime type conversions, and checked arithmetic/I/O helpers.
#include "Value.h"

#include <algorithm>
#include <sstream>
#include <utility>

namespace gash {

// --- ListValue implementation ---

ListValue::ListValue()
    : elementType_(Type::Int()), declaredCapacity_(0) {}

ListValue::ListValue(Type elementType, std::int64_t initialSize, std::size_t capacityHint)
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

Type ListValue::elementType() const noexcept {
    return elementType_;
}

Type ListValue::type() const {
    return Type::ListOf(elementType_);
}

std::size_t ListValue::size() const noexcept {
    return slots_.size();
}

std::size_t ListValue::capacity() const noexcept {
    return std::max(slots_.capacity(), declaredCapacity_);
}

std::size_t ListValue::declaredCapacity() const noexcept {
    return declaredCapacity_;
}

void ListValue::setDeclaredCapacity(std::size_t cap) noexcept {
    declaredCapacity_ = std::max(cap, slots_.size());
}

std::size_t ListValue::checkedIndex(std::int64_t index) const {
    if (index < 0 || static_cast<std::size_t>(index) >= slots_.size()) {
        throw std::out_of_range(
            "List index out of bounds: " + std::to_string(index));
    }
    return static_cast<std::size_t>(index);
}

bool ListValue::isInitialized(std::int64_t index) const {
    return slots_[checkedIndex(index)].has_value();
}

Value ListValue::get(std::int64_t index) const {
    const auto& slot = slots_[checkedIndex(index)];
    if (!slot.has_value()) {
        throw std::runtime_error(
            "Read of uninitialized list element at index " + std::to_string(index));
    }
    return std::visit([](const auto& elem) { return Value(elem); }, *slot);
}

void ListValue::set(std::int64_t index, const Value& value) {
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

void ListValue::resize(std::int64_t newSize) {
    if (newSize < 0) {
        throw std::runtime_error("List size cannot be negative");
    }
    const auto target = static_cast<std::size_t>(newSize);
    slots_.resize(target);
    if (target > declaredCapacity_) {
        declaredCapacity_ = target;
    }
}

bool ListValue::operator==(const ListValue& other) const {
    return elementType_ == other.elementType_ &&
           declaredCapacity_ == other.declaredCapacity_ &&
           slots_ == other.slots_;
}

bool ListValue::operator!=(const ListValue& other) const {
    return !(*this == other);
}

// --- Value implementation ---

Value::Value(int v) : data_(v) {}
Value::Value(double v) : data_(v) {}
Value::Value(bool v) : data_(v) {}
Value::Value(char v) : data_(v) {}
Value::Value(std::string v) : data_(std::move(v)) {}
Value::Value(const char* v) : data_(std::string(v ? v : "")) {}
Value::Value(ListValue v) : data_(std::move(v)) {}

Value Value::Int(int v) { return Value(v); }
Value Value::Float(double v) { return Value(v); }
Value Value::Bool(bool v) { return Value(v); }
Value Value::Char(char v) { return Value(v); }
Value Value::String(std::string v) { return Value(std::move(v)); }

Value Value::List(Type elementType, std::int64_t size, std::size_t capacity) {
    return Value(ListValue(elementType, size, capacity));
}

bool Value::isInitialized() const noexcept {
    return !std::holds_alternative<std::monostate>(data_);
}

bool Value::isInt() const noexcept { return std::holds_alternative<int>(data_); }
bool Value::isFloat() const noexcept { return std::holds_alternative<double>(data_); }
bool Value::isBool() const noexcept { return std::holds_alternative<bool>(data_); }
bool Value::isChar() const noexcept { return std::holds_alternative<char>(data_); }
bool Value::isString() const noexcept { return std::holds_alternative<std::string>(data_); }
bool Value::isList() const noexcept { return std::holds_alternative<ListValue>(data_); }

Type Value::type() const {
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

int Value::asInt() const {
    if (const auto* p = std::get_if<int>(&data_)) {
        return *p;
    }
    throw std::runtime_error(
        "Expected 'int' value, got '" + type().toString() + "'");
}

double Value::asFloat() const {
    if (const auto* p = std::get_if<double>(&data_)) {
        return *p;
    }
    throw std::runtime_error(
        "Expected 'float' value, got '" + type().toString() + "'");
}

bool Value::asBool() const {
    if (const auto* p = std::get_if<bool>(&data_)) {
        return *p;
    }
    throw std::runtime_error(
        "Expected 'bool' value, got '" + type().toString() + "'");
}

char Value::asChar() const {
    if (const auto* p = std::get_if<char>(&data_)) {
        return *p;
    }
    throw std::runtime_error(
        "Expected 'char' value, got '" + type().toString() + "'");
}

const std::string& Value::asString() const {
    if (const auto* p = std::get_if<std::string>(&data_)) {
        return *p;
    }
    throw std::runtime_error(
        "Expected 'string' value, got '" + type().toString() + "'");
}

const ListValue& Value::asList() const {
    if (const auto* p = std::get_if<ListValue>(&data_)) {
        return *p;
    }
    throw std::runtime_error(
        "Expected 'list' value, got '" + type().toString() + "'");
}

ListValue& Value::asList() {
    if (auto* p = std::get_if<ListValue>(&data_)) {
        return *p;
    }
    throw std::runtime_error(
        "Expected 'list' value, got '" + type().toString() + "'");
}

Value Value::castToInt() const {
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

Value Value::castToFloat() const {
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

Value Value::castToString() const {
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

std::string Value::toString() const {
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

bool Value::operator==(const Value& other) const {
    return data_ == other.data_;
}

bool Value::operator!=(const Value& other) const {
    return !(*this == other);
}

std::size_t Value::bindingId() const noexcept {
    return bindingId_;
}

void Value::setBindingId(std::size_t id) noexcept {
    bindingId_ = id;
}

// --- Runtime conversion and I/O helpers ---

// Reads one line from `in` for GASH `input()`.
std::string input(std::istream& in) {
    std::string line;
    if (!std::getline(in, line)) {
        return "";
    }
    return line;
}

int toInt(int v) { return v; }
int toInt(double v) { return static_cast<int>(v); }
int toInt(bool v) { return v ? 1 : 0; }
int toInt(char v) { return static_cast<int>(v); }
int toInt(const std::string& v) { return Value::String(v).castToInt().asInt(); }
int toInt(const char* v) { return toInt(std::string(v ? v : "")); }
int toInt(const Value& v) { return v.castToInt().asInt(); }

double toFloat(int v) { return static_cast<double>(v); }
double toFloat(double v) { return v; }
double toFloat(bool v) { return v ? 1.0 : 0.0; }
double toFloat(char v) { return static_cast<double>(v); }
double toFloat(const std::string& v) { return Value::String(v).castToFloat().asFloat(); }
double toFloat(const char* v) { return toFloat(std::string(v ? v : "")); }
double toFloat(const Value& v) { return v.castToFloat().asFloat(); }

std::string toString(int v) { return std::to_string(v); }
std::string toString(double v) {
    std::ostringstream oss;
    oss << v;
    return oss.str();
}
std::string toString(bool v) { return v ? "true" : "false"; }
std::string toString(char v) { return std::string(1, v); }
std::string toString(const std::string& v) { return v; }
std::string toString(const char* v) { return std::string(v ? v : ""); }
std::string toString(const Value& v) { return v.castToString().asString(); }

namespace detail {

[[noreturn]] void throwDivisionByZero() {
    throw std::runtime_error("Division by zero");
}

[[noreturn]] void throwDivisionOverflow() {
    throw std::runtime_error("Integer division overflow");
}

[[noreturn]] void throwInvalidExponentiation() {
    throw std::runtime_error("Invalid exponentiation operation");
}

} // namespace detail

// Performs integer modulo while rejecting division-by-zero and overflow at runtime.
int safeModulo(int lhs, int rhs) {
    if (rhs == 0) {
        throw std::runtime_error("Modulo by zero");
    }
    if (lhs == std::numeric_limits<int>::min() && rhs == -1) {
        throw std::runtime_error("Integer modulo overflow");
    }
    return lhs % rhs;
}

template int safeDivide<int>(int, int);
template double safeDivide<double>(double, double);
template int safePower<int>(int, int);
template double safePower<double>(double, double);
template void writeOutput<int>(std::ostream&, const int&);
template void writeOutput<double>(std::ostream&, const double&);
template void writeOutput<bool>(std::ostream&, const bool&);
template void writeOutput<char>(std::ostream&, const char&);
template void writeOutput<std::string>(std::ostream&, const std::string&);
template void writeOutput<Value>(std::ostream&, const Value&);

} // namespace gash

#ifndef GASH_TYPE_H
#define GASH_TYPE_H

#include "AST.h"

#include <cstddef>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

namespace gash {

// Core data types in the GASH type system.
enum class TypeKind {
    Int,
    Float,
    Bool,
    Char,
    String,
    List,
    Void,
    Unknown
};

// Represents a resolved GASH type, including parameterized list<T> types.
struct Type {
    TypeKind kind = TypeKind::Unknown;
    TypeKind elementKind = TypeKind::Unknown;

    constexpr Type() = default;

    constexpr explicit Type(TypeKind k, TypeKind elem = TypeKind::Unknown)
        : kind(k), elementKind(elem) {}

    // Factory methods for primitive and special types.
    static constexpr Type Int() { return Type(TypeKind::Int); }
    static constexpr Type Float() { return Type(TypeKind::Float); }
    static constexpr Type Bool() { return Type(TypeKind::Bool); }
    static constexpr Type Char() { return Type(TypeKind::Char); }
    static constexpr Type String() { return Type(TypeKind::String); }
    static constexpr Type Void() { return Type(TypeKind::Void); }
    static constexpr Type Unknown() { return Type(TypeKind::Unknown); }

    static Type ListOf(TypeKind elemKind) {
        if (!isPrimitiveKind(elemKind)) {
            throw std::invalid_argument("List element type must be a primitive GASH type");
        }
        return Type(TypeKind::List, elemKind);
    }

    static Type ListOf(const Type& elemType) {
        return ListOf(elemType.kind);
    }

    static constexpr bool isPrimitiveKind(TypeKind k) {
        return k == TypeKind::Int ||
               k == TypeKind::Float ||
               k == TypeKind::Bool ||
               k == TypeKind::Char ||
               k == TypeKind::String;
    }

    // Type classification helpers.
    constexpr bool isValid() const { return kind != TypeKind::Unknown; }
    constexpr bool isVoid() const { return kind == TypeKind::Void; }
    constexpr bool isPrimitive() const { return isPrimitiveKind(kind); }
    constexpr bool isNumeric() const {
        return kind == TypeKind::Int || kind == TypeKind::Float;
    }
    constexpr bool isIntegral() const { return kind == TypeKind::Int; }
    constexpr bool isBoolean() const { return kind == TypeKind::Bool; }
    constexpr bool isList() const {
        return kind == TypeKind::List && isPrimitiveKind(elementKind);
    }

    constexpr Type elementType() const {
        if (kind != TypeKind::List) {
            return Type::Unknown();
        }
        return Type(elementKind);
    }

    // Parse a type string such as "int", "float", or "list<int>".
    static Type fromName(std::string_view name) {
        if (name == "int") return Type::Int();
        if (name == "float") return Type::Float();
        if (name == "bool") return Type::Bool();
        if (name == "char") return Type::Char();
        if (name == "string") return Type::String();
        if (name == "void") return Type::Void();

        constexpr std::string_view prefix = "list<";
        if (name.size() > prefix.size() + 1 &&
            name.substr(0, prefix.size()) == prefix &&
            name.back() == '>') {
            std::string_view inner =
                name.substr(prefix.size(), name.size() - prefix.size() - 1);
            Type elem = fromName(inner);
            if (elem.isPrimitive()) {
                return Type::ListOf(elem.kind);
            }
        }
        return Type::Unknown();
    }

    // Convert type to canonical GASH type name.
    std::string toString() const {
        switch (kind) {
            case TypeKind::Int:     return "int";
            case TypeKind::Float:   return "float";
            case TypeKind::Bool:    return "bool";
            case TypeKind::Char:    return "char";
            case TypeKind::String:  return "string";
            case TypeKind::Void:    return "void";
            case TypeKind::List:
                return "list<" + Type(elementKind).toString() + ">";
            case TypeKind::Unknown:
            default:
                return "unknown";
        }
    }

    // Map GASH type to its C++17 representation (Assumption A27).
    std::string toCppType() const {
        switch (kind) {
            case TypeKind::Int:     return "int";
            case TypeKind::Float:   return "double";
            case TypeKind::Bool:    return "bool";
            case TypeKind::Char:    return "char";
            case TypeKind::String:  return "std::string";
            case TypeKind::Void:    return "void";
            case TypeKind::List:
                return "gash::List<" + Type(elementKind).toCppType() + ">";
            case TypeKind::Unknown:
            default:
                throw std::runtime_error("Cannot map unknown GASH type to C++ type");
        }
    }

    constexpr bool operator==(const Type& other) const {
        if (kind != other.kind) {
            return false;
        }
        if (kind == TypeKind::List) {
            return elementKind == other.elementKind;
        }
        return true;
    }

    constexpr bool operator!=(const Type& other) const {
        return !(*this == other);
    }
};

// Categories of symbols tracked during semantic analysis.
enum class SymbolKind {
    Variable,
    Parameter,
    Function
};

// Symbol table entry for a variable, parameter, or function.
struct Symbol {
    std::string name;
    SymbolKind kind = SymbolKind::Variable;
    Type type = Type::Unknown();
    std::vector<Type> paramTypes;
    std::size_t scopeId = 0;
    bool isLocked = false;
    bool isGuarded = false;
    bool hasReturnValue = false;
    SourceLocation location;

    Symbol() = default;

    Symbol(
        std::string n,
        SymbolKind k,
        Type t,
        std::vector<Type> params = {},
        SourceLocation loc = {}
    )
        : name(std::move(n)),
          kind(k),
          type(t),
          paramTypes(std::move(params)),
          location(loc) {}
};

// Lexical scope stack managing symbol tables across nested blocks and functions.
class ScopeStack {
public:
    ScopeStack() {
        // Initialize with global scope (scopeId = 1).
        pushScope();
    }

    void pushScope() {
        const std::size_t id = nextScopeId_++;
        scopes_.push_back(ScopeFrame{id, {}});
    }

    void popScope() {
        if (scopes_.size() <= 1) {
            throw std::logic_error("Cannot pop the global scope");
        }
        scopes_.pop_back();
    }

    std::size_t depth() const noexcept {
        return scopes_.size();
    }

    std::size_t currentScopeId() const noexcept {
        return scopes_.back().id;
    }

    // Declare a symbol in the current innermost scope.
    // Returns false if a symbol with the same name already exists in this scope.
    bool declare(Symbol symbol) {
        auto& current = scopes_.back();
        if (current.symbols.find(symbol.name) != current.symbols.end()) {
            return false;
        }
        symbol.scopeId = current.id;
        current.symbols.emplace(symbol.name, std::move(symbol));
        return true;
    }

    // Look up a symbol only in the current innermost scope.
    const Symbol* lookupCurrent(const std::string& name) const {
        const auto& current = scopes_.back();
        const auto it = current.symbols.find(name);
        if (it != current.symbols.end()) {
            return &it->second;
        }
        return nullptr;
    }

    // Look up a symbol from the innermost scope outward to the global scope.
    const Symbol* lookup(const std::string& name) const {
        for (auto it = scopes_.rbegin(); it != scopes_.rend(); ++it) {
            const auto found = it->symbols.find(name);
            if (found != it->symbols.end()) {
                return &found->second;
            }
        }
        return nullptr;
    }

    // Look up a mutable symbol from the innermost scope outward.
    Symbol* lookupMutable(const std::string& name) {
        for (auto it = scopes_.rbegin(); it != scopes_.rend(); ++it) {
            const auto found = it->symbols.find(name);
            if (found != it->symbols.end()) {
                return &found->second;
            }
        }
        return nullptr;
    }

private:
    struct ScopeFrame {
        std::size_t id = 0;
        std::unordered_map<std::string, Symbol> symbols;
    };

    std::vector<ScopeFrame> scopes_;
    std::size_t nextScopeId_ = 1;
};

} // namespace gash

#endif // GASH_TYPE_H

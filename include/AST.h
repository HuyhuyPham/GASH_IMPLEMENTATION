
#pragma once

#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace gash {

// Stores the position of a node in the GASH source code.
struct SourceLocation {
    int line = 0;
    int column = 0;
};

// Base class for all AST nodes.
struct Node {
    SourceLocation location;

    // Allows derived nodes to be destroyed safely.
    virtual ~Node() = default;
};

// Base class for expressions that represent values.
struct Expr : Node {
};

// Base class for statements that represent program actions.
struct Stmt : Node {
};

// Represents an integer literal, such as 10.
struct IntegerExpr final : Expr {
    int value;

    explicit IntegerExpr(int v)
        : value(v) {}
};

// Represents a variable reference, such as x.
struct VariableExpr final : Expr {
    std::string name;

    explicit VariableExpr(std::string n)
        : name(std::move(n)) {}
};

// Represents a binary operation, such as x + 5.
struct BinaryExpr final : Expr {
    std::string op;

    // Each child expression is owned by this node.
    std::unique_ptr<Expr> left;
    std::unique_ptr<Expr> right;

    BinaryExpr(
        std::string symbol,
        std::unique_ptr<Expr> lhs,
        std::unique_ptr<Expr> rhs
    )
        : op(std::move(symbol)),
          left(std::move(lhs)),
          right(std::move(rhs)) {}
};


 // Represents a unary operation, such as -x or !ready.
struct UnaryExpr final : Expr {
    std::string op;
    std::unique_ptr<Expr> operand;

    UnaryExpr(
        std::string symbol,
        std::unique_ptr<Expr> expr
    )
        : op(std::move(symbol)),
          operand(std::move(expr)) {}
};

// Represents a variable declaration, such as var x = 10;
struct VarDeclStmt final : Stmt {
    std::string name;

    // Stores the expression used to initialize the variable.
    std::unique_ptr<Expr> initializer;

    VarDeclStmt(
        std::string n,
        std::unique_ptr<Expr> init
    )
        : name(std::move(n)),
          initializer(std::move(init)) {}
};

// Represents the whole GASH program as a list of statements.
struct Program {
    std::vector<std::unique_ptr<Stmt>> statements;
};

} // namespace gash
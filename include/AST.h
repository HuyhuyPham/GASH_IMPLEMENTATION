
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

 // Represents a floating-point literal, such as 19.99.
struct FloatExpr final : Expr {
    double value;

    explicit FloatExpr(double v)
        : value(v) {}
};

// Represents a string literal, such as "Alice".
struct StringExpr final : Expr {
    std::string value;

    explicit StringExpr(std::string v)
        : value(std::move(v)) {}
};

// Represents a Boolean literal, such as true or false.
struct BoolExpr final : Expr {
    bool value;

    explicit BoolExpr(bool v)
        : value(v) {}
};

// Represents a character literal, such as 'A'.
struct CharExpr final : Expr {
    char value;

    explicit CharExpr(char v)
        : value(v) {}
};

// Represents a variable reference, such as x.
struct VariableExpr final : Expr {
    std::string name;

    explicit VariableExpr(std::string n)
        : name(std::move(n)) {}
};

// Represents a function call, such as add(2, 3).
struct CallExpr final : Expr {
    std::string callee;
    std::vector<std::unique_ptr<Expr>> arguments;

    CallExpr(
        std::string name,
        std::vector<std::unique_ptr<Expr>> args
    )
        : callee(std::move(name)),
          arguments(std::move(args)) {}
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

// Represents an assignment, such as x = x + 1.
struct AssignmentExpr final : Expr {
    std::unique_ptr<Expr> target;
    std::unique_ptr<Expr> value;

    AssignmentExpr(
        std::unique_ptr<Expr> assignmentTarget,
        std::unique_ptr<Expr> assignedValue
    )
        : target(std::move(assignmentTarget)),
          value(std::move(assignedValue)) {}
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

// Represents an expression used as a statement, such as x = 5;.
struct ExpressionStmt final : Stmt {
    std::unique_ptr<Expr> expression;

    explicit ExpressionStmt(std::unique_ptr<Expr> expr)
        : expression(std::move(expr)) {}
};

// Represents a block of statements enclosed in { }.
struct BlockStmt final : Stmt {
    std::vector<std::unique_ptr<Stmt>> statements;

    explicit BlockStmt(
        std::vector<std::unique_ptr<Stmt>> stmts
    )
        : statements(std::move(stmts)) {}
};

// Represents an if statement with an optional otherwise block.
struct IfStmt final : Stmt {
    std::unique_ptr<Expr> condition;
    std::unique_ptr<BlockStmt> thenBranch;
    std::unique_ptr<BlockStmt> elseBranch;

    IfStmt(
        std::unique_ptr<Expr> cond,
        std::unique_ptr<BlockStmt> thenBlock,
        std::unique_ptr<BlockStmt> elseBlock
    )
        : condition(std::move(cond)),
          thenBranch(std::move(thenBlock)),
          elseBranch(std::move(elseBlock)) {}
};

// Represents a while loop with a condition and body.
struct WhileStmt final : Stmt {
    std::unique_ptr<Expr> condition;
    std::unique_ptr<BlockStmt> body;

    WhileStmt(
        std::unique_ptr<Expr> cond,
        std::unique_ptr<BlockStmt> loopBody
    )
        : condition(std::move(cond)),
          body(std::move(loopBody)) {}
};

// Represents a repeat loop, such as repeat(3) { ... }.
struct RepeatStmt final : Stmt {
    std::unique_ptr<Expr> count;
    std::unique_ptr<BlockStmt> body;

    RepeatStmt(
        std::unique_ptr<Expr> repeatCount,
        std::unique_ptr<BlockStmt> loopBody
    )
        : count(std::move(repeatCount)),
          body(std::move(loopBody)) {}
};

// Represents the whole GASH program as a list of statements.
struct Program {
    std::vector<std::unique_ptr<Stmt>> statements;
};

} // namespace gash
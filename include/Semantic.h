#ifndef GASH_SEMANTIC_H
#define GASH_SEMANTIC_H

#include "AST.h"
#include "Type.h"

#include <cstddef>
#include <optional>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace gash {

// Exception thrown when semantic validation or type checking fails.
class SemanticError : public std::runtime_error {
public:
    SemanticError(const std::string& message, SourceLocation loc = {})
        : std::runtime_error(formatMessage(message, loc)),
          location_(loc) {}

    int line() const noexcept { return location_.line; }
    int column() const noexcept { return location_.column; }
    SourceLocation location() const noexcept { return location_; }

private:
    SourceLocation location_;

    static std::string formatMessage(const std::string& msg, SourceLocation loc) {
        if (loc.line > 0) {
            return "Semantic error at line " + std::to_string(loc.line) +
                   ", column " + std::to_string(loc.column) + ": " + msg;
        }
        return "Semantic error: " + msg;
    }
};

// Performs static semantic analysis, symbol resolution, and strict type checking.
class SemanticAnalyzer {
public:
    SemanticAnalyzer() = default;

    // Validate an entire GASH program.
    void analyze(const Program& program);

    // Validate a single statement in the current scope context.
    void analyzeStatement(const Stmt& stmt);

    // Infer and validate the type of an expression in the current scope context.
    Type inferType(const Expr& expr);

    // Access the current symbol scope stack.
    const ScopeStack& scopes() const noexcept { return scopes_; }
    ScopeStack& scopes() noexcept { return scopes_; }

    // Look up a symbol in the current scope chain.
    const Symbol* lookup(const std::string& name) const {
        return scopes_.lookup(name);
    }

    // Retrieve the inferred return type of a declared function (Assumption A29).
    Type getFunctionReturnType(const std::string& functionName) const;

private:
    struct FunctionContext {
        std::string name;
        std::optional<Type> inferredReturnType;
    };

    void preInferFunctionReturnType(const Stmt& stmt, FunctionContext& ctx);

    struct CheckpointRecord {
        std::string label;
        std::vector<std::string> variables;
        std::vector<Type> types;
        std::vector<std::size_t> scopeIds;
    };

    ScopeStack scopes_;
    std::size_t loopDepth_ = 0;
    std::vector<FunctionContext> functionStack_;
    std::unordered_map<std::string, CheckpointRecord> checkpoints_;

    // Statement analysis helpers
    void analyzeBlock(const BlockStmt& stmt, bool createNewScope = true);
    void analyzeVarDecl(const VarDeclStmt& stmt);
    void analyzeExpressionStmt(const ExpressionStmt& stmt);
    void analyzeReturnStmt(const ReturnStmt& stmt);
    void analyzeIfStmt(const IfStmt& stmt);
    void analyzeWhileStmt(const WhileStmt& stmt);
    void analyzeRepeatStmt(const RepeatStmt& stmt);
    void analyzeExitStmt(const ExitStmt& stmt);
    void analyzeSkipStmt(const SkipStmt& stmt);
    void analyzeFunctionDecl(const FunctionDeclStmt& stmt);
    void analyzeGuardStmt(const GuardStmt& stmt);
    void analyzeLockStmt(const LockStmt& stmt);
    void analyzeUnlockStmt(const UnlockStmt& stmt);
    void analyzeCheckpointStmt(const CheckpointStmt& stmt);
    void analyzeRestoreStmt(const RestoreStmt& stmt);
    void analyzeBudgetStmt(const BudgetStmt& stmt);

    // Expression type inference helpers
    Type inferVariableExpr(const VariableExpr& expr);
    Type inferCallExpr(const CallExpr& expr);
    Type inferListExpr(const ListExpr& expr);
    Type inferIndexExpr(const IndexExpr& expr);
    Type inferUnaryExpr(const UnaryExpr& expr);
    Type inferBinaryExpr(const BinaryExpr& expr);
    Type inferAssignmentExpr(const AssignmentExpr& expr);

    static bool isReservedBuiltinName(const std::string& name);
};

} // namespace gash

#endif // GASH_SEMANTIC_H

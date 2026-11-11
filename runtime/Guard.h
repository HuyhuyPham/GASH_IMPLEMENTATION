#ifndef GASH_GUARD_H
#define GASH_GUARD_H

#if __has_include("../include/AST.h")
#include "../include/AST.h"
#include "../include/Type.h"
#else
#include "AST.h"
#include "Type.h"
#endif
#include "Value.h"

#include <cstddef>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace gash {

// Callback used to resolve variable values when evaluating guard expressions.
using VariableResolver = std::function<Value(const std::string& name)>;

// Predicate taking a candidate Value and returning whether the mutation is allowed.
using GuardPredicate = std::function<bool(const Value& candidate)>;

// Evaluator taking a candidate Value and returning a typed GASH Value (must be bool).
using TypedGuardCallback = std::function<Value(const Value& candidate)>;

// Multi-variable guard callback evaluated against a candidate variable resolver.
using EnvironmentGuardCallback = std::function<Value(const VariableResolver& resolver)>;

// Represents a single mutation guard bound to one or more variables.
struct GuardRule {
    std::string targetVariable;
    std::size_t scopeId = 0;
    std::size_t declaredScopeId = 0;
    std::string description;
    TypedGuardCallback candidateEvaluator;
    EnvironmentGuardCallback envEvaluator;
    std::vector<std::string> referencedVariables;
};

// Manages variable mutation guards, type validation, initial checks, and candidate checks.
class GuardManager {
public:
    GuardManager() = default;

    // Register a boolean predicate guard on a single variable and validate its initial value.
    void addPredicateGuard(
        const std::string& variableName,
        std::size_t variableScopeId,
        const Value& currentValue,
        GuardPredicate predicate,
        std::string description = "",
        std::size_t declaredScopeId = 0
    );

    // Register a typed guard callback (must return Type::Bool()) and validate initial value.
    void addTypedGuard(
        const std::string& variableName,
        std::size_t variableScopeId,
        const Value& currentValue,
        TypedGuardCallback evaluator,
        std::string description = "",
        std::size_t declaredScopeId = 0
    );

    // Register an AST expression guard (e.g., from `guard x >= 0;`), validating its
    // boolean type and initial truth value against the current environment state.
    void addExpressionGuard(
        const Expr& condition,
        std::size_t variableScopeId,
        const VariableResolver& currentResolver,
        std::size_t declaredScopeId = 0
    );

    // Validate a single candidate assignment to `variableName`.
    // Throws std::runtime_error if any guard rejects the candidate value.
    void validateCandidate(
        const std::string& variableName,
        std::size_t scopeId,
        const Value& candidateValue,
        const VariableResolver& baseResolver = nullptr
    ) const;

    // Validate all active guards against a complete candidate state (used for atomic restore).
    void validateAll(
        const VariableResolver& candidateResolver
    ) const;

    // Check whether a variable currently has at least one active guard.
    bool hasGuard(const std::string& variableName, std::size_t scopeId = 0) const;

    // Remove all guards associated with a popped lexical scope.
    void removeGuardsForScope(std::size_t scopeId);

    // Clear all registered guards.
    void clear() noexcept { rules_.clear(); }

    // Number of active guard rules.
    std::size_t size() const noexcept { return rules_.size(); }

    // Evaluate an AST expression using a variable resolver, enforcing strict GASH types.
    static Value evaluateExpression(
        const Expr& expr,
        const VariableResolver& resolver
    );

    // Collect all variable names referenced inside an AST expression.
    static std::vector<std::string> collectVariables(const Expr& expr);

private:
    std::vector<GuardRule> rules_;

    static bool ensureBooleanResult(
        const Value& result,
        const std::string& context
    );
};

} // namespace gash

#endif // GASH_GUARD_H

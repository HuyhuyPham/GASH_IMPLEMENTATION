#include "Guard.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <unordered_set>

namespace gash {

namespace {

// Deep-copy an AST expression so registered expression guards own their AST safely.
std::unique_ptr<Expr> cloneExpr(const Expr& expr) {
    if (const auto* intNode = dynamic_cast<const IntegerExpr*>(&expr)) {
        auto copy = std::make_unique<IntegerExpr>(intNode->value);
        copy->location = intNode->location;
        return copy;
    }
    if (const auto* floatNode = dynamic_cast<const FloatExpr*>(&expr)) {
        auto copy = std::make_unique<FloatExpr>(floatNode->value);
        copy->location = floatNode->location;
        return copy;
    }
    if (const auto* strNode = dynamic_cast<const StringExpr*>(&expr)) {
        auto copy = std::make_unique<StringExpr>(strNode->value);
        copy->location = strNode->location;
        return copy;
    }
    if (const auto* boolNode = dynamic_cast<const BoolExpr*>(&expr)) {
        auto copy = std::make_unique<BoolExpr>(boolNode->value);
        copy->location = boolNode->location;
        return copy;
    }
    if (const auto* charNode = dynamic_cast<const CharExpr*>(&expr)) {
        auto copy = std::make_unique<CharExpr>(charNode->value);
        copy->location = charNode->location;
        return copy;
    }
    if (const auto* varNode = dynamic_cast<const VariableExpr*>(&expr)) {
        auto copy = std::make_unique<VariableExpr>(varNode->name);
        copy->location = varNode->location;
        return copy;
    }
    if (const auto* unaryNode = dynamic_cast<const UnaryExpr*>(&expr)) {
        auto copy = std::make_unique<UnaryExpr>(
            unaryNode->op,
            cloneExpr(*unaryNode->operand));
        copy->location = unaryNode->location;
        return copy;
    }
    if (const auto* binNode = dynamic_cast<const BinaryExpr*>(&expr)) {
        auto copy = std::make_unique<BinaryExpr>(
            binNode->op,
            cloneExpr(*binNode->left),
            cloneExpr(*binNode->right));
        copy->location = binNode->location;
        return copy;
    }
    if (const auto* idxNode = dynamic_cast<const IndexExpr*>(&expr)) {
        auto copy = std::make_unique<IndexExpr>(
            cloneExpr(*idxNode->collection),
            cloneExpr(*idxNode->index));
        copy->location = idxNode->location;
        return copy;
    }
    if (const auto* callNode = dynamic_cast<const CallExpr*>(&expr)) {
        std::vector<std::unique_ptr<Expr>> clonedArgs;
        clonedArgs.reserve(callNode->arguments.size());
        for (const auto& arg : callNode->arguments) {
            clonedArgs.push_back(cloneExpr(*arg));
        }
        auto copy = std::make_unique<CallExpr>(
            callNode->callee,
            std::move(clonedArgs));
        copy->location = callNode->location;
        return copy;
    }
    if (const auto* listNode = dynamic_cast<const ListExpr*>(&expr)) {
        auto copy = std::make_unique<ListExpr>(
            listNode->elementType,
            cloneExpr(*listNode->size));
        copy->location = listNode->location;
        return copy;
    }
    throw std::runtime_error("Unsupported expression node in guard condition");
}

void collectVarsRecursive(
    const Expr& expr,
    std::vector<std::string>& out,
    std::unordered_set<std::string>& seen
) {
    if (const auto* varNode = dynamic_cast<const VariableExpr*>(&expr)) {
        if (seen.insert(varNode->name).second) {
            out.push_back(varNode->name);
        }
        return;
    }
    if (const auto* unaryNode = dynamic_cast<const UnaryExpr*>(&expr)) {
        if (unaryNode->operand) {
            collectVarsRecursive(*unaryNode->operand, out, seen);
        }
        return;
    }
    if (const auto* binNode = dynamic_cast<const BinaryExpr*>(&expr)) {
        if (binNode->left) {
            collectVarsRecursive(*binNode->left, out, seen);
        }
        if (binNode->right) {
            collectVarsRecursive(*binNode->right, out, seen);
        }
        return;
    }
    if (const auto* idxNode = dynamic_cast<const IndexExpr*>(&expr)) {
        if (idxNode->collection) {
            collectVarsRecursive(*idxNode->collection, out, seen);
        }
        if (idxNode->index) {
            collectVarsRecursive(*idxNode->index, out, seen);
        }
        return;
    }
    if (const auto* callNode = dynamic_cast<const CallExpr*>(&expr)) {
        for (const auto& arg : callNode->arguments) {
            if (arg) {
                collectVarsRecursive(*arg, out, seen);
            }
        }
        return;
    }
}

} // anonymous namespace

bool GuardManager::ensureBooleanResult(
    const Value& result,
    const std::string& context
) {
    if (!result.isInitialized() || result.type() != Type::Bool()) {
        throw std::runtime_error(
            "Guard condition in " + context + " must evaluate to 'bool', got '" +
            result.type().toString() + "'");
    }
    return result.asBool();
}

void GuardManager::addPredicateGuard(
    const std::string& variableName,
    std::size_t variableScopeId,
    const Value& currentValue,
    GuardPredicate predicate,
    std::string description,
    std::size_t declaredScopeId
) {
    if (!predicate) {
        throw std::runtime_error("Guard predicate cannot be empty");
    }
    addTypedGuard(
        variableName,
        variableScopeId,
        currentValue,
        [pred = std::move(predicate)](const Value& candidate) -> Value {
            return Value::Bool(pred(candidate));
        },
        std::move(description),
        declaredScopeId);
}

void GuardManager::addTypedGuard(
    const std::string& variableName,
    std::size_t variableScopeId,
    const Value& currentValue,
    TypedGuardCallback evaluator,
    std::string description,
    std::size_t declaredScopeId
) {
    if (variableName.empty()) {
        throw std::runtime_error("Guard variable name cannot be empty");
    }
    if (!evaluator) {
        throw std::runtime_error("Guard evaluator cannot be empty");
    }
    if (!currentValue.isInitialized()) {
        throw std::runtime_error(
            "Cannot attach guard to uninitialized variable '" + variableName + "'");
    }

    // Validate guard return type and initial variable value immediately.
    Value initialCheck = evaluator(currentValue);
    if (!ensureBooleanResult(initialCheck, "guard on '" + variableName + "'")) {
        throw std::runtime_error(
            "Initial value of variable '" + variableName +
            "' violates guard condition");
    }

    GuardRule rule;
    rule.targetVariable = variableName;
    rule.scopeId = variableScopeId;
    rule.declaredScopeId = (declaredScopeId != 0) ? declaredScopeId : variableScopeId;
    rule.description = std::move(description);
    rule.candidateEvaluator = std::move(evaluator);
    rule.referencedVariables = {variableName};

    rules_.push_back(std::move(rule));
}

void GuardManager::addExpressionGuard(
    const Expr& condition,
    std::size_t variableScopeId,
    const VariableResolver& currentResolver,
    std::size_t declaredScopeId
) {
    if (!currentResolver) {
        throw std::runtime_error("Guard expression requires a variable resolver");
    }

    std::vector<std::string> vars = collectVariables(condition);
    auto sharedExpr = std::shared_ptr<const Expr>(cloneExpr(condition));

    EnvironmentGuardCallback envEval =
        [sharedExpr](const VariableResolver& resolver) -> Value {
            return evaluateExpression(*sharedExpr, resolver);
        };

    // Validate type (must be bool) and initial values immediately.
    Value initialResult = envEval(currentResolver);
    if (!ensureBooleanResult(initialResult, "expression guard")) {
        throw std::runtime_error(
            "Initial state violates guard expression");
    }

    GuardRule rule;
    if (vars.size() == 1) {
        rule.targetVariable = vars[0];
    }
    rule.scopeId = variableScopeId;
    rule.declaredScopeId = (declaredScopeId != 0) ? declaredScopeId : variableScopeId;
    rule.envEvaluator = std::move(envEval);
    rule.referencedVariables = std::move(vars);

    rules_.push_back(std::move(rule));
}

void GuardManager::validateCandidate(
    const std::string& variableName,
    std::size_t scopeId,
    const Value& candidateValue,
    const VariableResolver& baseResolver
) const {
    if (!candidateValue.isInitialized()) {
        throw std::runtime_error(
            "Candidate value for variable '" + variableName + "' is uninitialized");
    }

    for (const auto& rule : rules_) {
        const bool scopeMatches =
            (rule.scopeId == 0 || scopeId == 0 || rule.scopeId == scopeId);

        if (rule.candidateEvaluator &&
            rule.targetVariable == variableName &&
            scopeMatches) {
            Value res = rule.candidateEvaluator(candidateValue);
            if (!ensureBooleanResult(res, "guard on '" + variableName + "'")) {
                throw std::runtime_error(
                    "Guard rejected assignment to variable '" + variableName + "'");
            }
        }

        if (rule.envEvaluator) {
            const bool touchesVar =
                rule.referencedVariables.empty() ||
                std::find(
                    rule.referencedVariables.begin(),
                    rule.referencedVariables.end(),
                    variableName) != rule.referencedVariables.end();

            if (touchesVar && scopeMatches) {
                VariableResolver candidateResolver =
                    [&](const std::string& name) -> Value {
                        if (name == variableName) {
                            return candidateValue;
                        }
                        if (baseResolver) {
                            return baseResolver(name);
                        }
                        throw std::runtime_error(
                            "Cannot resolve variable '" + name + "' during guard check");
                    };

                Value res = rule.envEvaluator(candidateResolver);
                if (!ensureBooleanResult(res, "guard on '" + variableName + "'")) {
                    throw std::runtime_error(
                        "Guard rejected assignment to variable '" + variableName + "'");
                }
            }
        }
    }
}

void GuardManager::validateAll(const VariableResolver& candidateResolver) const {
    if (!candidateResolver) {
        throw std::runtime_error("Candidate resolver cannot be null");
    }

    for (const auto& rule : rules_) {
        if (rule.candidateEvaluator && !rule.targetVariable.empty()) {
            Value candidateVal = candidateResolver(rule.targetVariable);
            Value res = rule.candidateEvaluator(candidateVal);
            if (!ensureBooleanResult(res, "guard on '" + rule.targetVariable + "'")) {
                throw std::runtime_error(
                    "Guard rejected state for variable '" + rule.targetVariable + "'");
            }
        }
        if (rule.envEvaluator) {
            Value res = rule.envEvaluator(candidateResolver);
            if (!ensureBooleanResult(res, "expression guard")) {
                throw std::runtime_error("Guard rejected restored environment state");
            }
        }
    }
}

bool GuardManager::hasGuard(
    const std::string& variableName,
    std::size_t scopeId
) const {
    for (const auto& rule : rules_) {
        const bool scopeMatches =
            (rule.scopeId == 0 || scopeId == 0 || rule.scopeId == scopeId);
        if (!scopeMatches) {
            continue;
        }
        if (rule.targetVariable == variableName) {
            return true;
        }
        if (std::find(
                rule.referencedVariables.begin(),
                rule.referencedVariables.end(),
                variableName) != rule.referencedVariables.end()) {
            return true;
        }
    }
    return false;
}

void GuardManager::removeGuardsForScope(std::size_t scopeId) {
    rules_.erase(
        std::remove_if(
            rules_.begin(),
            rules_.end(),
            [scopeId](const GuardRule& rule) {
                return rule.scopeId == scopeId || rule.declaredScopeId == scopeId;
            }),
        rules_.end());
}

std::vector<std::string> GuardManager::collectVariables(const Expr& expr) {
    std::vector<std::string> vars;
    std::unordered_set<std::string> seen;
    collectVarsRecursive(expr, vars, seen);
    return vars;
}

Value GuardManager::evaluateExpression(
    const Expr& expr,
    const VariableResolver& resolver
) {
    if (const auto* intNode = dynamic_cast<const IntegerExpr*>(&expr)) {
        return Value::Int(intNode->value);
    }
    if (const auto* floatNode = dynamic_cast<const FloatExpr*>(&expr)) {
        return Value::Float(floatNode->value);
    }
    if (const auto* strNode = dynamic_cast<const StringExpr*>(&expr)) {
        return Value::String(strNode->value);
    }
    if (const auto* boolNode = dynamic_cast<const BoolExpr*>(&expr)) {
        return Value::Bool(boolNode->value);
    }
    if (const auto* charNode = dynamic_cast<const CharExpr*>(&expr)) {
        return Value::Char(charNode->value);
    }
    if (const auto* varNode = dynamic_cast<const VariableExpr*>(&expr)) {
        if (!resolver) {
            throw std::runtime_error(
                "No resolver available for variable '" + varNode->name + "'");
        }
        Value v = resolver(varNode->name);
        if (!v.isInitialized()) {
            throw std::runtime_error(
                "Variable '" + varNode->name + "' is uninitialized in guard");
        }
        return v;
    }
    if (const auto* idxNode = dynamic_cast<const IndexExpr*>(&expr)) {
        Value coll = evaluateExpression(*idxNode->collection, resolver);
        Value idx = evaluateExpression(*idxNode->index, resolver);
        if (!coll.isList()) {
            throw std::runtime_error("Cannot index non-list value in guard");
        }
        if (!idx.isInt()) {
            throw std::runtime_error("List index in guard must be 'int'");
        }
        return coll.asList().get(idx.asInt());
    }
    if (const auto* callNode = dynamic_cast<const CallExpr*>(&expr)) {
        if (callNode->callee == "int" && callNode->arguments.size() == 1) {
            return evaluateExpression(*callNode->arguments[0], resolver).castToInt();
        }
        if (callNode->callee == "float" && callNode->arguments.size() == 1) {
            return evaluateExpression(*callNode->arguments[0], resolver).castToFloat();
        }
        if (callNode->callee == "string" && callNode->arguments.size() == 1) {
            return evaluateExpression(*callNode->arguments[0], resolver).castToString();
        }
        throw std::runtime_error(
            "Unsupported function call '" + callNode->callee + "' in guard");
    }
    if (const auto* unaryNode = dynamic_cast<const UnaryExpr*>(&expr)) {
        Value opVal = evaluateExpression(*unaryNode->operand, resolver);
        if (unaryNode->op == "-") {
            if (opVal.isInt()) return Value::Int(-opVal.asInt());
            if (opVal.isFloat()) return Value::Float(-opVal.asFloat());
            throw std::runtime_error("Unary '-' in guard requires 'int' or 'float'");
        }
        if (unaryNode->op == "!") {
            if (opVal.isBool()) return Value::Bool(!opVal.asBool());
            throw std::runtime_error("Unary '!' in guard requires 'bool'");
        }
        throw std::runtime_error("Unsupported unary operator '" + unaryNode->op + "'");
    }
    if (const auto* binNode = dynamic_cast<const BinaryExpr*>(&expr)) {
        // Short-circuit logical operators while still enforcing strict bool types.
        if (binNode->op == "&&" || binNode->op == "||") {
            Value lhs = evaluateExpression(*binNode->left, resolver);
            Value rhs = evaluateExpression(*binNode->right, resolver);
            if (!lhs.isBool() || !rhs.isBool()) {
                throw std::runtime_error(
                    "Logical operator '" + binNode->op + "' requires 'bool' operands");
            }
            if (binNode->op == "&&") {
                return Value::Bool(lhs.asBool() && rhs.asBool());
            }
            return Value::Bool(lhs.asBool() || rhs.asBool());
        }

        Value lhs = evaluateExpression(*binNode->left, resolver);
        Value rhs = evaluateExpression(*binNode->right, resolver);
        if (lhs.type() != rhs.type()) {
            throw std::runtime_error(
                "Strict type mismatch in guard binary operator '" + binNode->op +
                "': '" + lhs.type().toString() + "' vs '" + rhs.type().toString() + "'");
        }

        const std::string& op = binNode->op;
        if (op == "==") return Value::Bool(lhs == rhs);
        if (op == "!=") return Value::Bool(lhs != rhs);

        if (lhs.isInt()) {
            const int a = lhs.asInt();
            const int b = rhs.asInt();
            if (op == "+") return Value::Int(a + b);
            if (op == "-") return Value::Int(a - b);
            if (op == "*") return Value::Int(a * b);
            if (op == "/") {
                if (b == 0) throw std::runtime_error("Division by zero in guard");
                return Value::Int(a / b);
            }
            if (op == "%") {
                if (b == 0) throw std::runtime_error("Modulo by zero in guard");
                return Value::Int(a % b);
            }
            if (op == "^") {
                return Value::Int(static_cast<int>(std::pow(a, b)));
            }
            if (op == "<") return Value::Bool(a < b);
            if (op == "<=") return Value::Bool(a <= b);
            if (op == ">") return Value::Bool(a > b);
            if (op == ">=") return Value::Bool(a >= b);
        } else if (lhs.isFloat()) {
            const double a = lhs.asFloat();
            const double b = rhs.asFloat();
            if (op == "+") return Value::Float(a + b);
            if (op == "-") return Value::Float(a - b);
            if (op == "*") return Value::Float(a * b);
            if (op == "/") {
                if (b == 0.0) throw std::runtime_error("Division by zero in guard");
                return Value::Float(a / b);
            }
            if (op == "^") return Value::Float(std::pow(a, b));
            if (op == "<") return Value::Bool(a < b);
            if (op == "<=") return Value::Bool(a <= b);
            if (op == ">") return Value::Bool(a > b);
            if (op == ">=") return Value::Bool(a >= b);
        } else if (lhs.isString()) {
            const std::string& a = lhs.asString();
            const std::string& b = rhs.asString();
            if (op == "+") return Value::String(a + b);
            if (op == "<") return Value::Bool(a < b);
            if (op == "<=") return Value::Bool(a <= b);
            if (op == ">") return Value::Bool(a > b);
            if (op == ">=") return Value::Bool(a >= b);
        } else if (lhs.isChar()) {
            const char a = lhs.asChar();
            const char b = rhs.asChar();
            if (op == "<") return Value::Bool(a < b);
            if (op == "<=") return Value::Bool(a <= b);
            if (op == ">") return Value::Bool(a > b);
            if (op == ">=") return Value::Bool(a >= b);
        }

        throw std::runtime_error(
            "Operator '" + op + "' is not supported for type '" +
            lhs.type().toString() + "' in guard");
    }

    throw std::runtime_error("Invalid expression in guard evaluation");
}

} // namespace gash

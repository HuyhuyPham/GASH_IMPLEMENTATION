#if __has_include("../include/Semantic.h")
#include "../include/Semantic.h"
#else
#include "Semantic.h"
#endif

#include <unordered_set>

namespace gash {

bool SemanticAnalyzer::isReservedBuiltinName(const std::string& name) {
    return name == "input" ||
           name == "output" ||
           name == "inspect" ||
           name == "preview" ||
           name == "extend" ||
           name == "int" ||
           name == "float" ||
           name == "string" ||
           name == "bool" ||
           name == "char" ||
           name == "list";
}

Type SemanticAnalyzer::getFunctionReturnType(const std::string& functionName) const {
    const Symbol* sym = scopes_.lookup(functionName);
    if (!sym || sym->kind != SymbolKind::Function) {
        throw SemanticError("Unknown function '" + functionName + "'");
    }
    return sym->type;
}

void SemanticAnalyzer::preInferFunctionReturnType(
    const Stmt& stmt,
    FunctionContext& ctx
) {
    if (ctx.inferredReturnType.has_value()) {
        return;
    }
    if (const auto* retStmt = dynamic_cast<const ReturnStmt*>(&stmt)) {
        if (retStmt->value) {
            try {
                Type t = inferType(*retStmt->value);
                if (t.isValid() && !t.isVoid()) {
                    ctx.inferredReturnType = t;
                    if (Symbol* fnSym = scopes_.lookupMutable(ctx.name)) {
                        fnSym->type = t;
                        fnSym->hasReturnValue = true;
                    }
                }
            } catch (const SemanticError&) {
                // Ignore during pre-pass if the return expression depends on
                // local variables or recursive return types not yet resolved.
            }
        }
        return;
    }
    if (const auto* blockStmt = dynamic_cast<const BlockStmt*>(&stmt)) {
        for (const auto& inner : blockStmt->statements) {
            if (inner) {
                preInferFunctionReturnType(*inner, ctx);
                if (ctx.inferredReturnType.has_value()) return;
            }
        }
        return;
    }
    if (const auto* ifStmt = dynamic_cast<const IfStmt*>(&stmt)) {
        if (ifStmt->thenBranch) {
            preInferFunctionReturnType(*ifStmt->thenBranch, ctx);
        }
        if (!ctx.inferredReturnType.has_value() && ifStmt->elseBranch) {
            preInferFunctionReturnType(*ifStmt->elseBranch, ctx);
        }
        return;
    }
    if (const auto* whileStmt = dynamic_cast<const WhileStmt*>(&stmt)) {
        if (whileStmt->body) {
            preInferFunctionReturnType(*whileStmt->body, ctx);
        }
        return;
    }
    if (const auto* repeatStmt = dynamic_cast<const RepeatStmt*>(&stmt)) {
        if (repeatStmt->body) {
            preInferFunctionReturnType(*repeatStmt->body, ctx);
        }
        return;
    }
    if (const auto* budgetStmt = dynamic_cast<const BudgetStmt*>(&stmt)) {
        if (budgetStmt->body) {
            preInferFunctionReturnType(*budgetStmt->body, ctx);
        }
        return;
    }
}

void SemanticAnalyzer::analyze(const Program& program) {
    for (const auto& stmt : program.statements) {
        if (!stmt) {
            throw SemanticError("Null statement in program");
        }
        analyzeStatement(*stmt);
    }
}

void SemanticAnalyzer::analyzeStatement(const Stmt& stmt) {
    if (const auto* block = dynamic_cast<const BlockStmt*>(&stmt)) {
        analyzeBlock(*block, true);
        return;
    }
    if (const auto* varDecl = dynamic_cast<const VarDeclStmt*>(&stmt)) {
        analyzeVarDecl(*varDecl);
        return;
    }
    if (const auto* exprStmt = dynamic_cast<const ExpressionStmt*>(&stmt)) {
        analyzeExpressionStmt(*exprStmt);
        return;
    }
    if (const auto* retStmt = dynamic_cast<const ReturnStmt*>(&stmt)) {
        analyzeReturnStmt(*retStmt);
        return;
    }
    if (const auto* ifStmt = dynamic_cast<const IfStmt*>(&stmt)) {
        analyzeIfStmt(*ifStmt);
        return;
    }
    if (const auto* whileStmt = dynamic_cast<const WhileStmt*>(&stmt)) {
        analyzeWhileStmt(*whileStmt);
        return;
    }
    if (const auto* repeatStmt = dynamic_cast<const RepeatStmt*>(&stmt)) {
        analyzeRepeatStmt(*repeatStmt);
        return;
    }
    if (const auto* exitStmt = dynamic_cast<const ExitStmt*>(&stmt)) {
        analyzeExitStmt(*exitStmt);
        return;
    }
    if (const auto* skipStmt = dynamic_cast<const SkipStmt*>(&stmt)) {
        analyzeSkipStmt(*skipStmt);
        return;
    }
    if (const auto* funcDecl = dynamic_cast<const FunctionDeclStmt*>(&stmt)) {
        analyzeFunctionDecl(*funcDecl);
        return;
    }
    if (const auto* guardStmt = dynamic_cast<const GuardStmt*>(&stmt)) {
        analyzeGuardStmt(*guardStmt);
        return;
    }
    if (const auto* lockStmt = dynamic_cast<const LockStmt*>(&stmt)) {
        analyzeLockStmt(*lockStmt);
        return;
    }
    if (const auto* unlockStmt = dynamic_cast<const UnlockStmt*>(&stmt)) {
        analyzeUnlockStmt(*unlockStmt);
        return;
    }
    if (const auto* cpStmt = dynamic_cast<const CheckpointStmt*>(&stmt)) {
        analyzeCheckpointStmt(*cpStmt);
        return;
    }
    if (const auto* restoreStmt = dynamic_cast<const RestoreStmt*>(&stmt)) {
        analyzeRestoreStmt(*restoreStmt);
        return;
    }
    if (const auto* budgetStmt = dynamic_cast<const BudgetStmt*>(&stmt)) {
        analyzeBudgetStmt(*budgetStmt);
        return;
    }

    throw SemanticError("Unsupported statement node", stmt.location);
}

void SemanticAnalyzer::analyzeBlock(const BlockStmt& stmt, bool createNewScope) {
    if (createNewScope) {
        scopes_.pushScope();
    }
    for (const auto& inner : stmt.statements) {
        if (!inner) {
            if (createNewScope) {
                scopes_.popScope();
            }
            throw SemanticError("Null statement inside block", stmt.location);
        }
        analyzeStatement(*inner);
    }
    if (createNewScope) {
        scopes_.popScope();
    }
}

void SemanticAnalyzer::analyzeVarDecl(const VarDeclStmt& stmt) {
    if (stmt.name.empty()) {
        throw SemanticError("Variable name cannot be empty", stmt.location);
    }
    if (isReservedBuiltinName(stmt.name)) {
        throw SemanticError(
            "Cannot declare variable with reserved name '" + stmt.name + "'",
            stmt.location);
    }
    if (!stmt.initializer) {
        throw SemanticError(
            "Variable '" + stmt.name + "' requires an initializer expression",
            stmt.location);
    }

    Type initType = inferType(*stmt.initializer);
    if (!initType.isValid() || initType.isVoid()) {
        throw SemanticError(
            "Cannot initialize variable '" + stmt.name + "' with type '" +
                initType.toString() + "'",
            stmt.location);
    }

    Symbol sym;
    sym.name = stmt.name;
    sym.kind = SymbolKind::Variable;
    sym.type = initType;
    sym.location = stmt.location;

    if (!scopes_.declare(std::move(sym))) {
        throw SemanticError(
            "Redeclaration of symbol '" + stmt.name + "' in the same scope",
            stmt.location);
    }
}

void SemanticAnalyzer::analyzeExpressionStmt(const ExpressionStmt& stmt) {
    if (!stmt.expression) {
        throw SemanticError("Empty expression statement", stmt.location);
    }
    inferType(*stmt.expression);
}

void SemanticAnalyzer::analyzeReturnStmt(const ReturnStmt& stmt) {
    if (functionStack_.empty()) {
        throw SemanticError(
            "'return' statement is only allowed inside a function",
            stmt.location);
    }
    if (!stmt.value) {
        throw SemanticError(
            "'return' statement requires a value expression",
            stmt.location);
    }

    Type returnType = inferType(*stmt.value);
    if (!returnType.isValid() || returnType.isVoid()) {
        throw SemanticError(
            "Function cannot return a value of type '" + returnType.toString() + "'",
            stmt.location);
    }

    auto& currentFunc = functionStack_.back();
    if (!currentFunc.inferredReturnType.has_value()) {
        currentFunc.inferredReturnType = returnType;
        if (Symbol* fnSym = scopes_.lookupMutable(currentFunc.name)) {
            fnSym->type = returnType;
            fnSym->hasReturnValue = true;
        }
    } else if (*currentFunc.inferredReturnType != returnType) {
        throw SemanticError(
            "Inconsistent return type in function '" + currentFunc.name +
                "': expected '" + currentFunc.inferredReturnType->toString() +
                "', got '" + returnType.toString() + "'",
            stmt.location);
    }
}

void SemanticAnalyzer::analyzeIfStmt(const IfStmt& stmt) {
    if (!stmt.condition) {
        throw SemanticError("If statement requires a condition", stmt.location);
    }
    Type condType = inferType(*stmt.condition);
    if (condType != Type::Bool()) {
        throw SemanticError(
            "If condition must have type 'bool', got '" + condType.toString() + "'",
            stmt.condition->location);
    }

    if (!stmt.thenBranch) {
        throw SemanticError("If statement requires a then-block", stmt.location);
    }
    analyzeBlock(*stmt.thenBranch, true);

    if (stmt.elseBranch) {
        analyzeBlock(*stmt.elseBranch, true);
    }
}

void SemanticAnalyzer::analyzeWhileStmt(const WhileStmt& stmt) {
    if (!stmt.condition) {
        throw SemanticError("While loop requires a condition", stmt.location);
    }
    Type condType = inferType(*stmt.condition);
    if (condType != Type::Bool()) {
        throw SemanticError(
            "While condition must have type 'bool', got '" + condType.toString() + "'",
            stmt.condition->location);
    }

    if (!stmt.body) {
        throw SemanticError("While loop requires a body block", stmt.location);
    }

    ++loopDepth_;
    try {
        analyzeBlock(*stmt.body, true);
    } catch (...) {
        --loopDepth_;
        throw;
    }
    --loopDepth_;
}

void SemanticAnalyzer::analyzeRepeatStmt(const RepeatStmt& stmt) {
    if (!stmt.count) {
        throw SemanticError("Repeat loop requires a count expression", stmt.location);
    }
    Type countType = inferType(*stmt.count);
    if (countType != Type::Int()) {
        throw SemanticError(
            "Repeat count must have type 'int', got '" + countType.toString() + "'",
            stmt.count->location);
    }

    if (!stmt.body) {
        throw SemanticError("Repeat loop requires a body block", stmt.location);
    }

    ++loopDepth_;
    try {
        analyzeBlock(*stmt.body, true);
    } catch (...) {
        --loopDepth_;
        throw;
    }
    --loopDepth_;
}

void SemanticAnalyzer::analyzeExitStmt(const ExitStmt& stmt) {
    if (loopDepth_ == 0) {
        throw SemanticError(
            "'exit' statement is only allowed inside a loop",
            stmt.location);
    }
}

void SemanticAnalyzer::analyzeSkipStmt(const SkipStmt& stmt) {
    if (loopDepth_ == 0) {
        throw SemanticError(
            "'skip' statement is only allowed inside a loop",
            stmt.location);
    }
}

void SemanticAnalyzer::analyzeFunctionDecl(const FunctionDeclStmt& stmt) {
    if (stmt.name.empty()) {
        throw SemanticError("Function name cannot be empty", stmt.location);
    }
    if (isReservedBuiltinName(stmt.name)) {
        throw SemanticError(
            "Cannot declare function with reserved name '" + stmt.name + "'",
            stmt.location);
    }
    if (!stmt.body) {
        throw SemanticError(
            "Function '" + stmt.name + "' requires a body block",
            stmt.location);
    }

    std::vector<Type> paramTypes;
    paramTypes.reserve(stmt.parameters.size());
    std::unordered_set<std::string> seenParams;

    for (const auto& param : stmt.parameters) {
        if (param.name.empty()) {
            throw SemanticError(
                "Parameter name cannot be empty in function '" + stmt.name + "'",
                stmt.location);
        }
        if (isReservedBuiltinName(param.name)) {
            throw SemanticError(
                "Parameter cannot use reserved name '" + param.name + "'",
                stmt.location);
        }
        if (!seenParams.insert(param.name).second) {
            throw SemanticError(
                "Duplicate parameter name '" + param.name + "' in function '" +
                    stmt.name + "'",
                stmt.location);
        }

        Type pType = Type::fromName(param.type);
        if (!pType.isValid() || pType.isVoid()) {
            throw SemanticError(
                "Invalid parameter type '" + param.type + "' for parameter '" +
                    param.name + "' in function '" + stmt.name + "'",
                stmt.location);
        }
        paramTypes.push_back(pType);
    }

    Symbol funcSym;
    funcSym.name = stmt.name;
    funcSym.kind = SymbolKind::Function;
    funcSym.type = Type::Void();
    funcSym.paramTypes = paramTypes;
    funcSym.hasReturnValue = false;
    funcSym.location = stmt.location;

    if (!scopes_.declare(std::move(funcSym))) {
        throw SemanticError(
            "Redeclaration of function or symbol '" + stmt.name + "' in the same scope",
            stmt.location);
    }

    // Create the function scope containing parameters and body statements.
    scopes_.pushScope();
    functionStack_.push_back(FunctionContext{stmt.name, std::nullopt});
    const std::size_t savedLoopDepth = loopDepth_;
    loopDepth_ = 0;

    try {
        for (std::size_t i = 0; i < stmt.parameters.size(); ++i) {
            Symbol paramSym;
            paramSym.name = stmt.parameters[i].name;
            paramSym.kind = SymbolKind::Parameter;
            paramSym.type = paramTypes[i];
            paramSym.location = stmt.location;
            scopes_.declare(std::move(paramSym));
        }

        // Pre-infer return type from base-case return statements to support recursion (A29).
        preInferFunctionReturnType(*stmt.body, functionStack_.back());

        analyzeBlock(*stmt.body, false);
    } catch (...) {
        loopDepth_ = savedLoopDepth;
        functionStack_.pop_back();
        scopes_.popScope();
        throw;
    }

    const auto inferred = functionStack_.back().inferredReturnType;
    loopDepth_ = savedLoopDepth;
    functionStack_.pop_back();
    scopes_.popScope();

    if (Symbol* declaredFn = scopes_.lookupMutable(stmt.name)) {
        if (inferred.has_value()) {
            declaredFn->type = *inferred;
            declaredFn->hasReturnValue = true;
        } else {
            declaredFn->type = Type::Void();
            declaredFn->hasReturnValue = false;
        }
    }
}

void SemanticAnalyzer::analyzeGuardStmt(const GuardStmt& stmt) {
    if (!stmt.condition) {
        throw SemanticError("Guard statement requires a condition", stmt.location);
    }
    Type condType = inferType(*stmt.condition);
    if (condType != Type::Bool()) {
        throw SemanticError(
            "Guard condition must have type 'bool', got '" + condType.toString() + "'",
            stmt.condition->location);
    }
}

void SemanticAnalyzer::analyzeLockStmt(const LockStmt& stmt) {
    Symbol* sym = scopes_.lookupMutable(stmt.name);
    if (!sym) {
        throw SemanticError(
            "Cannot lock undeclared variable '" + stmt.name + "'",
            stmt.location);
    }
    if (sym->kind == SymbolKind::Function) {
        throw SemanticError(
            "Cannot lock function '" + stmt.name + "'",
            stmt.location);
    }
    sym->isLocked = true;
}

void SemanticAnalyzer::analyzeUnlockStmt(const UnlockStmt& stmt) {
    Symbol* sym = scopes_.lookupMutable(stmt.name);
    if (!sym) {
        throw SemanticError(
            "Cannot unlock undeclared variable '" + stmt.name + "'",
            stmt.location);
    }
    if (sym->kind == SymbolKind::Function) {
        throw SemanticError(
            "Cannot unlock function '" + stmt.name + "'",
            stmt.location);
    }
    sym->isLocked = false;
}

void SemanticAnalyzer::analyzeCheckpointStmt(const CheckpointStmt& stmt) {
    if (stmt.label.empty()) {
        throw SemanticError("Checkpoint label cannot be empty", stmt.location);
    }
    if (checkpoints_.find(stmt.label) != checkpoints_.end()) {
        throw SemanticError(
            "Duplicate checkpoint label '" + stmt.label + "'",
            stmt.location);
    }
    if (stmt.variables.empty()) {
        throw SemanticError(
            "Checkpoint '" + stmt.label + "' requires at least one variable",
            stmt.location);
    }

    CheckpointRecord record;
    record.label = stmt.label;
    std::unordered_set<std::string> seen;

    for (const auto& varName : stmt.variables) {
        if (!seen.insert(varName).second) {
            throw SemanticError(
                "Duplicate variable '" + varName + "' in checkpoint '" + stmt.label + "'",
                stmt.location);
        }
        const Symbol* sym = scopes_.lookup(varName);
        if (!sym) {
            throw SemanticError(
                "Checkpoint '" + stmt.label + "' references undeclared variable '" +
                    varName + "'",
                stmt.location);
        }
        if (sym->kind == SymbolKind::Function) {
            throw SemanticError(
                "Checkpoint '" + stmt.label + "' cannot capture function '" +
                    varName + "'",
                stmt.location);
        }
        record.variables.push_back(varName);
        record.types.push_back(sym->type);
        record.scopeIds.push_back(sym->scopeId);
    }

    checkpoints_.emplace(stmt.label, std::move(record));
}

void SemanticAnalyzer::analyzeRestoreStmt(const RestoreStmt& stmt) {
    if (stmt.label.empty()) {
        throw SemanticError("Restore checkpoint label cannot be empty", stmt.location);
    }
    const auto it = checkpoints_.find(stmt.label);
    if (it == checkpoints_.end()) {
        throw SemanticError(
            "Cannot restore unknown checkpoint '" + stmt.label + "'",
            stmt.location);
    }

    const auto& record = it->second;
    for (std::size_t i = 0; i < record.variables.size(); ++i) {
        const auto& varName = record.variables[i];
        const Symbol* sym = scopes_.lookup(varName);
        if (!sym || sym->kind == SymbolKind::Function) {
            throw SemanticError(
                "Variable '" + varName + "' for checkpoint '" + stmt.label +
                    "' is no longer in scope",
                stmt.location);
        }
        if (sym->type != record.types[i]) {
            throw SemanticError(
                "Type mismatch restoring variable '" + varName + "' from checkpoint '" +
                    stmt.label + "'",
                stmt.location);
        }
        if (sym->scopeId != record.scopeIds[i]) {
            throw SemanticError(
                "Scope mismatch restoring variable '" + varName + "' from checkpoint '" +
                    stmt.label + "'",
                stmt.location);
        }
    }
}

void SemanticAnalyzer::analyzeBudgetStmt(const BudgetStmt& stmt) {
    if (!stmt.limit) {
        throw SemanticError("Budget statement requires a limit expression", stmt.location);
    }
    Type limitType = inferType(*stmt.limit);
    if (limitType != Type::Int()) {
        throw SemanticError(
            "Budget limit must have type 'int', got '" + limitType.toString() + "'",
            stmt.limit->location);
    }
    if (!stmt.body) {
        throw SemanticError("Budget statement requires a body block", stmt.location);
    }
    analyzeBlock(*stmt.body, true);
}

Type SemanticAnalyzer::inferType(const Expr& expr) {
    if (dynamic_cast<const IntegerExpr*>(&expr)) {
        return Type::Int();
    }
    if (dynamic_cast<const FloatExpr*>(&expr)) {
        return Type::Float();
    }
    if (dynamic_cast<const StringExpr*>(&expr)) {
        return Type::String();
    }
    if (dynamic_cast<const BoolExpr*>(&expr)) {
        return Type::Bool();
    }
    if (dynamic_cast<const CharExpr*>(&expr)) {
        return Type::Char();
    }
    if (const auto* varExpr = dynamic_cast<const VariableExpr*>(&expr)) {
        return inferVariableExpr(*varExpr);
    }
    if (const auto* callExpr = dynamic_cast<const CallExpr*>(&expr)) {
        return inferCallExpr(*callExpr);
    }
    if (const auto* listExpr = dynamic_cast<const ListExpr*>(&expr)) {
        return inferListExpr(*listExpr);
    }
    if (const auto* indexExpr = dynamic_cast<const IndexExpr*>(&expr)) {
        return inferIndexExpr(*indexExpr);
    }
    if (const auto* unaryExpr = dynamic_cast<const UnaryExpr*>(&expr)) {
        return inferUnaryExpr(*unaryExpr);
    }
    if (const auto* binaryExpr = dynamic_cast<const BinaryExpr*>(&expr)) {
        return inferBinaryExpr(*binaryExpr);
    }
    if (const auto* assignExpr = dynamic_cast<const AssignmentExpr*>(&expr)) {
        return inferAssignmentExpr(*assignExpr);
    }

    throw SemanticError("Unsupported expression node", expr.location);
}

Type SemanticAnalyzer::inferVariableExpr(const VariableExpr& expr) {
    const Symbol* sym = scopes_.lookup(expr.name);
    if (!sym) {
        throw SemanticError(
            "Use of undeclared identifier '" + expr.name + "'",
            expr.location);
    }
    if (sym->kind == SymbolKind::Function) {
        throw SemanticError(
            "Function '" + expr.name + "' cannot be used as a variable value",
            expr.location);
    }
    return sym->type;
}

Type SemanticAnalyzer::inferCallExpr(const CallExpr& expr) {
    // 1. Explicit type conversion calls: int(x), float(x), string(x)
    if (expr.callee == "int" || expr.callee == "float" || expr.callee == "string") {
        if (expr.arguments.size() != 1) {
            throw SemanticError(
                "Type conversion '" + expr.callee + "()' requires exactly 1 argument",
                expr.location);
        }
        Type argType = inferType(*expr.arguments[0]);
        if (!argType.isPrimitive()) {
            throw SemanticError(
                "Cannot convert type '" + argType.toString() + "' using '" +
                    expr.callee + "()'",
                expr.location);
        }
        return Type::fromName(expr.callee);
    }

    // 2. Built-in functions: input, output, inspect, preview, extend
    if (expr.callee == "input") {
        if (!expr.arguments.empty()) {
            throw SemanticError(
                "Built-in 'input()' takes 0 arguments",
                expr.location);
        }
        return Type::String();
    }

    if (expr.callee == "output") {
        if (expr.arguments.size() != 1) {
            throw SemanticError(
                "Built-in 'output()' requires exactly 1 argument",
                expr.location);
        }
        Type argType = inferType(*expr.arguments[0]);
        if (!argType.isValid() || argType.isVoid()) {
            throw SemanticError(
                "Cannot pass 'void' expression to 'output()'",
                expr.location);
        }
        return Type::Void();
    }

    if (expr.callee == "inspect") {
        if (expr.arguments.size() != 1) {
            throw SemanticError(
                "Built-in 'inspect()' requires exactly 1 argument",
                expr.location);
        }
        Type argType = inferType(*expr.arguments[0]);
        if (!argType.isList()) {
            throw SemanticError(
                "Built-in 'inspect()' requires a list argument, got '" +
                    argType.toString() + "'",
                expr.location);
        }
        return Type::Void();
    }

    if (expr.callee == "preview") {
        if (!expr.arguments.empty()) {
            throw SemanticError(
                "Built-in 'preview()' takes 0 arguments",
                expr.location);
        }
        return Type::Void();
    }

    if (expr.callee == "extend") {
        if (expr.arguments.size() != 2) {
            throw SemanticError(
                "Built-in 'extend()' requires exactly 2 arguments (list, newSize)",
                expr.location);
        }
        Type listType = inferType(*expr.arguments[0]);
        if (!listType.isList()) {
            throw SemanticError(
                "First argument to 'extend()' must be a list, got '" +
                    listType.toString() + "'",
                expr.location);
        }
        Type sizeType = inferType(*expr.arguments[1]);
        if (sizeType != Type::Int()) {
            throw SemanticError(
                "Second argument to 'extend()' must have type 'int', got '" +
                    sizeType.toString() + "'",
                expr.location);
        }
        return Type::Void();
    }

    // 3. User-defined function calls
    const Symbol* sym = scopes_.lookup(expr.callee);
    if (!sym) {
        throw SemanticError(
            "Call to undeclared function '" + expr.callee + "'",
            expr.location);
    }
    if (sym->kind != SymbolKind::Function) {
        throw SemanticError(
            "Identifier '" + expr.callee + "' is not a function",
            expr.location);
    }
    if (expr.arguments.size() != sym->paramTypes.size()) {
        throw SemanticError(
            "Function '" + expr.callee + "' expects " +
                std::to_string(sym->paramTypes.size()) + " argument(s), got " +
                std::to_string(expr.arguments.size()),
            expr.location);
    }

    for (std::size_t i = 0; i < expr.arguments.size(); ++i) {
        Type argType = inferType(*expr.arguments[i]);
        if (argType != sym->paramTypes[i]) {
            throw SemanticError(
                "Argument " + std::to_string(i + 1) + " of function '" +
                    expr.callee + "' expects type '" +
                    sym->paramTypes[i].toString() + "', got '" +
                    argType.toString() + "'",
                expr.arguments[i]->location);
        }
    }

    return sym->type;
}

Type SemanticAnalyzer::inferListExpr(const ListExpr& expr) {
    Type elemType = Type::fromName(expr.elementType);
    if (!elemType.isPrimitive()) {
        throw SemanticError(
            "Invalid list element type '" + expr.elementType + "'",
            expr.location);
    }
    if (!expr.size) {
        throw SemanticError("List creation requires a size expression", expr.location);
    }
    Type sizeType = inferType(*expr.size);
    if (sizeType != Type::Int()) {
        throw SemanticError(
            "List size expression must have type 'int', got '" +
                sizeType.toString() + "'",
            expr.size->location);
    }

    // Reject negative integer literal sizes statically.
    if (const auto* intLit = dynamic_cast<const IntegerExpr*>(expr.size.get())) {
        if (intLit->value < 0) {
            throw SemanticError("List size cannot be negative", expr.size->location);
        }
    } else if (const auto* unary = dynamic_cast<const UnaryExpr*>(expr.size.get())) {
        if (unary->op == "-") {
            if (const auto* innerLit = dynamic_cast<const IntegerExpr*>(unary->operand.get())) {
                if (innerLit->value > 0) {
                    throw SemanticError("List size cannot be negative", expr.size->location);
                }
            }
        }
    }

    return Type::ListOf(elemType);
}

Type SemanticAnalyzer::inferIndexExpr(const IndexExpr& expr) {
    if (!expr.collection || !expr.index) {
        throw SemanticError("Invalid index expression", expr.location);
    }
    Type collType = inferType(*expr.collection);
    if (!collType.isList()) {
        throw SemanticError(
            "Cannot index value of non-list type '" + collType.toString() + "'",
            expr.location);
    }
    Type indexType = inferType(*expr.index);
    if (indexType != Type::Int()) {
        throw SemanticError(
            "List index must have type 'int', got '" + indexType.toString() + "'",
            expr.index->location);
    }
    return collType.elementType();
}

Type SemanticAnalyzer::inferUnaryExpr(const UnaryExpr& expr) {
    if (!expr.operand) {
        throw SemanticError("Unary expression missing operand", expr.location);
    }
    Type operandType = inferType(*expr.operand);

    if (expr.op == "-") {
        if (!operandType.isNumeric()) {
            throw SemanticError(
                "Unary '-' requires 'int' or 'float' operand, got '" +
                    operandType.toString() + "'",
                expr.location);
        }
        return operandType;
    }

    if (expr.op == "!") {
        if (operandType != Type::Bool()) {
            throw SemanticError(
                "Logical '!' requires 'bool' operand, got '" +
                    operandType.toString() + "'",
                expr.location);
        }
        return Type::Bool();
    }

    throw SemanticError("Unknown unary operator '" + expr.op + "'", expr.location);
}

Type SemanticAnalyzer::inferBinaryExpr(const BinaryExpr& expr) {
    if (!expr.left || !expr.right) {
        throw SemanticError("Binary expression missing operand", expr.location);
    }
    Type lhs = inferType(*expr.left);
    Type rhs = inferType(*expr.right);

    // Strict typing: both sides must have the exact same type.
    if (lhs != rhs) {
        throw SemanticError(
            "Type mismatch for operator '" + expr.op + "': cannot combine '" +
                lhs.toString() + "' and '" + rhs.toString() + "'",
            expr.location);
    }

    // Addition (+): supports int + int, float + float, and string + string.
    if (expr.op == "+") {
        if (lhs.isNumeric() || lhs == Type::String()) {
            return lhs;
        }
        throw SemanticError(
            "Operator '+' is not supported for type '" + lhs.toString() + "'",
            expr.location);
    }

    // Arithmetic (-, *, /, ^): strictly int or float.
    if (expr.op == "-" || expr.op == "*" || expr.op == "/" || expr.op == "^") {
        if (lhs.isNumeric()) {
            return lhs;
        }
        throw SemanticError(
            "Arithmetic operator '" + expr.op + "' requires 'int' or 'float', got '" +
                lhs.toString() + "'",
            expr.location);
    }

    // Modulo (%): strictly int.
    if (expr.op == "%") {
        if (lhs == Type::Int()) {
            return Type::Int();
        }
        throw SemanticError(
            "Modulo operator '%' requires 'int' operands, got '" +
                lhs.toString() + "'",
            expr.location);
    }

    // Relational (<, <=, >, >=): int, float, char, or string -> bool.
    if (expr.op == "<" || expr.op == "<=" || expr.op == ">" || expr.op == ">=") {
        if (lhs.isNumeric() || lhs == Type::Char() || lhs == Type::String()) {
            return Type::Bool();
        }
        throw SemanticError(
            "Relational operator '" + expr.op + "' is not supported for type '" +
                lhs.toString() + "'",
            expr.location);
    }

    // Equality (==, !=): any matching valid non-void type -> bool.
    if (expr.op == "==" || expr.op == "!=") {
        if (!lhs.isValid() || lhs.isVoid()) {
            throw SemanticError(
                "Equality operator '" + expr.op + "' cannot compare type '" +
                    lhs.toString() + "'",
                expr.location);
        }
        return Type::Bool();
    }

    // Logical (&&, ||): strictly bool -> bool.
    if (expr.op == "&&" || expr.op == "||") {
        if (lhs == Type::Bool()) {
            return Type::Bool();
        }
        throw SemanticError(
            "Logical operator '" + expr.op + "' requires 'bool' operands, got '" +
                lhs.toString() + "'",
            expr.location);
    }

    throw SemanticError("Unknown binary operator '" + expr.op + "'", expr.location);
}

Type SemanticAnalyzer::inferAssignmentExpr(const AssignmentExpr& expr) {
    if (!expr.target || !expr.value) {
        throw SemanticError("Invalid assignment expression", expr.location);
    }

    Type valueType = inferType(*expr.value);
    if (!valueType.isValid() || valueType.isVoid()) {
        throw SemanticError(
            "Cannot assign expression of type '" + valueType.toString() + "'",
            expr.location);
    }

    if (const auto* varTarget = dynamic_cast<const VariableExpr*>(expr.target.get())) {
        const Symbol* sym = scopes_.lookup(varTarget->name);
        if (!sym) {
            throw SemanticError(
                "Assignment to undeclared variable '" + varTarget->name + "'",
                expr.location);
        }
        if (sym->kind == SymbolKind::Function) {
            throw SemanticError(
                "Cannot assign to function '" + varTarget->name + "'",
                expr.location);
        }
        if (sym->type != valueType) {
            throw SemanticError(
                "Type mismatch in assignment to '" + varTarget->name +
                    "': expected '" + sym->type.toString() + "', got '" +
                    valueType.toString() + "'",
                expr.location);
        }
        return sym->type;
    }

    if (const auto* indexTarget = dynamic_cast<const IndexExpr*>(expr.target.get())) {
        Type elemType = inferIndexExpr(*indexTarget);
        if (elemType != valueType) {
            throw SemanticError(
                "Type mismatch in list element assignment: expected '" +
                    elemType.toString() + "', got '" + valueType.toString() + "'",
                expr.location);
        }
        return elemType;
    }

    throw SemanticError("Invalid assignment target", expr.location);
}

} // namespace gash

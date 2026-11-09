#include "CodeGenerator.h"
#include "Semantic.h"

#include <cstddef>
#include <iomanip>
#include <iostream>
#include <stdexcept>

namespace gash
{
    // Formatting
    void CodeGenerator::emitLine(const std::string &line)
    {
        for (int i = 0; i < indentLevel; i++)
        {
            output << "    ";
        }
        output << line << std::endl;
    }

    // Statement generation
    void CodeGenerator::emitStatement(const Stmt &stmt)
    {
        if (const auto *decl = dynamic_cast<const VarDeclStmt *>(&stmt))
        {
            emitLine("gash::BudgetScope::tick();");
            if (!decl->initializer)
            {
                throw std::runtime_error("Variable declaration requires an initializer");
            }
            emitLine("auto gash_" + decl->name + " = " + emitExpression(*decl->initializer) + ";");
            emitLine("gash_env.bindReference(\"" + escapeString(decl->name) + "\", gash_" + decl->name + ");");
            return;
        }
        if (const auto *exprStmt = dynamic_cast<const ExpressionStmt *>(&stmt))
        {
            emitLine("gash::BudgetScope::tick();");
            if (const auto *call = dynamic_cast<const CallExpr *>(exprStmt->expression.get()))
            {
                if (call->callee == "output")
                {
                    if (call->arguments.size() != 1)
                    {
                        throw std::runtime_error("Initial output() implementation requires one argument");
                    }
                    emitLine("std::cout << " + emitExpression(*call->arguments[0]) + " << '\\n';");
                    return;
                }
            }
            emitLine(emitExpression(*exprStmt->expression) + ";");
            return;
        }
        if (const auto *blockStmt = dynamic_cast<const BlockStmt *>(&stmt))
        {
            emitLine("{");
            indentLevel++;
            int scopeId = nextScopeGuardId++;
            emitLine("gash::ScopeGuard gash_scope_" + std::to_string(scopeId) + "(gash_env);");
            for (std::size_t i = 0; i < blockStmt->statements.size(); i++)
            {
                emitStatement(*blockStmt->statements[i]);
            }
            indentLevel--;
            emitLine("}");
            return;
        }
        if (const auto *ifStmt = dynamic_cast<const IfStmt *>(&stmt))
        {
            emitLine("gash::BudgetScope::tick();");
            std::string condition = emitExpression(*ifStmt->condition);
            emitLine("if(" + condition + "){");
            indentLevel++;
            emitStatement(*ifStmt->thenBranch);
            indentLevel--;
            emitLine("}");
            if (ifStmt->elseBranch != nullptr)
            {
                emitLine("else {");
                indentLevel++;
                emitStatement(*ifStmt->elseBranch);
                indentLevel--;
                emitLine("}");
            }
            return;
        }
        if (const auto *whileStmt = dynamic_cast<const WhileStmt *>(&stmt))
        {
            emitLine("gash::BudgetScope::tick();");
            std::string condition = emitExpression(*whileStmt->condition);
            emitLine("while ((gash::BudgetScope::tick(), " + condition + ")) {");
            indentLevel++;
            emitStatement(*whileStmt->body);
            indentLevel--;
            emitLine("}");
            return;
        }
        if (const auto *repeatStmt = dynamic_cast<const RepeatStmt *>(&stmt))
        {
            emitLine("gash::BudgetScope::tick();");

            std::string times = emitExpression(*repeatStmt->count);

            int id = nextRepeatId++;
            std::string counterName = "gash_repeat_index_" + std::to_string(id);
            std::string limitName = "gash_repeat_limit_" + std::to_string(id);
            emitLine("{");
            indentLevel++;
            emitLine("const int " + limitName + " = " + times + ";");
            emitLine("for (int " + counterName + " = 0; (gash::BudgetScope::tick(), " + counterName + " < " + limitName + "); " + counterName + "++) {");
            indentLevel++;
            emitStatement(*repeatStmt->body);
            indentLevel--;
            emitLine("}");
            indentLevel--;
            emitLine("}");
            return;
        }
        if (dynamic_cast<const ExitStmt *>(&stmt))
        {
            emitLine("gash::BudgetScope::tick();");
            emitLine("break;");
            return;
        }
        if (dynamic_cast<const SkipStmt *>(&stmt))
        {
            emitLine("gash::BudgetScope::tick();");
            emitLine("continue;");
            return;
        }
        if (const auto *returnStmt = dynamic_cast<const ReturnStmt *>(&stmt))
        {
            emitLine("gash::BudgetScope::tick();");
            if (returnStmt->value == nullptr)
            {
                emitLine("return;");
            }
            else
            {
                std::string value = emitExpression(*returnStmt->value);
                emitLine("return " + value + ";");
            }
            return;
        }
        if (const auto *functionStmt = dynamic_cast<const FunctionDeclStmt *>(&stmt))
        {
            std::string name = "gash_" + functionStmt->name;
            std::string returnTypeCpp = "auto";
            auto foundRet = functionReturnTypes.find(functionStmt->name);
            if (foundRet != functionReturnTypes.end() && foundRet->second.isValid())
            {
                returnTypeCpp = foundRet->second.toCppType();
            }

            std::string signature = returnTypeCpp + " " + name + "(";
            for (std::size_t i = 0; i < functionStmt->parameters.size(); i++)
            {
                const auto &param = functionStmt->parameters[i];
                std::string cppType = mapType(param.type);
                if (i > 0)
                {
                    signature += ", ";
                }
                signature += cppType + " gash_" + param.name;
            }
            signature += ")";
            emitLine(signature);

            emitLine("{");
            indentLevel++;
            int scopeId = nextScopeGuardId++;
            emitLine("gash::ScopeGuard gash_scope_" + std::to_string(scopeId) + "(gash_env);");
            for (const auto &param : functionStmt->parameters)
            {
                emitLine("gash_env.bindReference(\"" + escapeString(param.name) + "\", gash_" + param.name + ");");
            }
            if (const auto *bodyBlock = dynamic_cast<const BlockStmt *>(functionStmt->body.get()))
            {
                for (std::size_t i = 0; i < bodyBlock->statements.size(); i++)
                {
                    emitStatement(*bodyBlock->statements[i]);
                }
            }
            else if (functionStmt->body)
            {
                emitStatement(*functionStmt->body);
            }
            indentLevel--;
            emitLine("}");
            return;
        }
        if (const auto *budget = dynamic_cast<const BudgetStmt *>(&stmt))
        {
            emitLine("{");
            indentLevel++;
            emitLine("gash::BudgetScope gash_budget(" + emitExpression(*budget->limit) + ");");
            emitLine("try {");
            indentLevel++;
            emitLine("gash::BudgetScope::check();");
            emitStatement(*budget->body);
            indentLevel--;
            emitLine("} catch (const gash::BudgetExceeded& error) {");
            indentLevel++;
            emitLine("if (!gash_budget.handles(error)) throw;");
            indentLevel--;
            emitLine("}");
            indentLevel--;
            emitLine("}");
            return;
        }
        if (const auto *guardStmt = dynamic_cast<const GuardStmt *>(&stmt))
        {
            emitLine("gash::BudgetScope::tick();");
            std::string cond = emitExpression(*guardStmt->condition);
            emitLine("gash_env.addLiveGuard([&]() -> bool { return (" + cond + "); });");
            return;
        }
        if (const auto *lockStmt = dynamic_cast<const LockStmt *>(&stmt))
        {
            emitLine("gash::BudgetScope::tick();");
            emitLine("gash_env.lock(\"" + escapeString(lockStmt->name) + "\");");
            return;
        }
        if (const auto *unlockStmt = dynamic_cast<const UnlockStmt *>(&stmt))
        {
            emitLine("gash::BudgetScope::tick();");
            emitLine("gash_env.unlock(\"" + escapeString(unlockStmt->name) + "\");");
            return;
        }
        if (const auto *cpStmt = dynamic_cast<const CheckpointStmt *>(&stmt))
        {
            emitLine("gash::BudgetScope::tick();");
            std::string varList = "{";
            for (std::size_t i = 0; i < cpStmt->variables.size(); ++i)
            {
                if (i > 0)
                {
                    varList += ", ";
                }
                varList += "\"" + escapeString(cpStmt->variables[i]) + "\"";
            }
            varList += "}";
            emitLine("gash_checkpoints.save(\"" + escapeString(cpStmt->label) + "\", " + varList + ", gash_env);");
            return;
        }
        if (const auto *restoreStmt = dynamic_cast<const RestoreStmt *>(&stmt))
        {
            emitLine("gash::BudgetScope::tick();");
            emitLine("gash_checkpoints.restore(\"" + escapeString(restoreStmt->label) + "\", gash_env);");
            return;
        }
        throw std::runtime_error("Statement type not yet supported by CodeGenerator");
    }

    // Expression generation
    std::string CodeGenerator::emitExpression(const Expr &expr) const
    {
        if (const auto *node = dynamic_cast<const IntegerExpr *>(&expr))
        {
            return std::to_string(node->value);
        }
        if (const auto *node = dynamic_cast<const FloatExpr *>(&expr))
        {
            std::ostringstream value;
            value << std::setprecision(17) << node->value;
            std::string s = value.str();
            if (s.find('.') == std::string::npos && s.find('e') == std::string::npos && s.find('E') == std::string::npos)
            {
                s += ".0";
            }
            return s;
        }
        if (const auto *node = dynamic_cast<const BoolExpr *>(&expr))
        {
            return node->value ? "true" : "false";
        }
        if (const auto *node = dynamic_cast<const CharExpr *>(&expr))
        {
            std::string ch(1, node->value);
            return "'" + escapeString(ch) + "'";
        }
        if (const auto *node = dynamic_cast<const StringExpr *>(&expr))
        {
            return "std::string(\"" + escapeString(node->value) + "\")";
        }
        if (const auto *node = dynamic_cast<const VariableExpr *>(&expr))
        {
            return "gash_" + node->name;
        }
        if (const auto *node = dynamic_cast<const BinaryExpr *>(&expr))
        {
            std::string left = emitExpression(*node->left);
            std::string right = emitExpression(*node->right);
            if (node->op == "^")
            {
                return "static_cast<std::decay_t<decltype(" + left + ")>>(std::pow(" + left + ", " + right + "))";
            }
            return "(" + left + " " + node->op + " " + right + ")";
        }
        if (const auto *node = dynamic_cast<const UnaryExpr *>(&expr))
        {
            return "(" + node->op + emitExpression(*node->operand) + ")";
        }
        if (const auto *node = dynamic_cast<const AssignmentExpr *>(&expr))
        {
            std::string value = emitExpression(*node->value);
            if (const auto *idxTarget = dynamic_cast<const IndexExpr *>(node->target.get()))
            {
                std::string collection = emitExpression(*idxTarget->collection);
                std::string index = emitExpression(*idxTarget->index);
                if (const auto *varColl = dynamic_cast<const VariableExpr *>(idxTarget->collection.get()))
                {
                    return "gash_env.mutateList(\"" + escapeString(varColl->name) + "\", [&]() { return ((" + collection + ").set(" + index + ", " + value + "), " + value + "); })";
                }
                return "((" + collection + ").set(" + index + ", " + value + "), " + value + ")";
            }
            std::string target = emitExpression(*node->target);
            if (const auto *varTarget = dynamic_cast<const VariableExpr *>(node->target.get()))
            {
                return "gash_env.mutateVar(\"" + escapeString(varTarget->name) + "\", [&]() { return (" + target + " = " + value + "); })";
            }
            return "(" + target + " = " + value + ")";
        }
        if (const auto *node = dynamic_cast<const CallExpr *>(&expr))
        {
            if (node->callee == "input")
            {
                if (!node->arguments.empty())
                {
                    throw std::runtime_error("input() expects exactly 0 arguments");
                }
                return "gash::input()";
            }
            if (node->callee == "int")
            {
                if (node->arguments.size() != 1)
                {
                    throw std::runtime_error("int() expects exactly 1 argument");
                }
                std::string argument = emitExpression(*node->arguments[0]);
                return "gash::toInt(" + argument + ")";
            }
            if (node->callee == "float")
            {
                if (node->arguments.size() != 1)
                {
                    throw std::runtime_error("float() expects exactly 1 argument");
                }
                std::string argument = emitExpression(*node->arguments[0]);
                return "gash::toFloat(" + argument + ")";
            }
            if (node->callee == "string")
            {
                if (node->arguments.size() != 1)
                {
                    throw std::runtime_error("string() expects exactly 1 argument");
                }
                std::string argument = emitExpression(*node->arguments[0]);
                return "gash::toString(" + argument + ")";
            }
            if (node->callee == "inspect")
            {
                if (node->arguments.size() != 1)
                {
                    throw std::runtime_error("inspect() expects exactly 1 argument");
                }
                std::string argument = emitExpression(*node->arguments[0]);
                return "gash::inspect(" + argument + ")";
            }
            if (node->callee == "preview")
            {
                if (node->arguments.size() > 0)
                {
                    throw std::runtime_error("preview() expects exactly 0 argument");
                }
                return "gash::preview()";
            }
            if (node->callee == "extend")
            {
                if (node->arguments.size() != 2)
                {
                    throw std::runtime_error("extend() expects exactly 2 arguments");
                }
                std::string listArg = emitExpression(*node->arguments[0]);
                std::string sizeArg = emitExpression(*node->arguments[1]);
                if (const auto *varList = dynamic_cast<const VariableExpr *>(node->arguments[0].get()))
                {
                    return "gash_env.mutateListVoid(\"" + escapeString(varList->name) + "\", [&]() { (" + listArg + ").resize(" + sizeArg + "); })";
                }
                return "(" + listArg + ").resize(" + sizeArg + ")";
            }
            std::string functionName = "gash_" + node->callee;
            std::string result = functionName + "(";
            for (std::size_t i = 0; i < node->arguments.size(); i++)
            {
                const auto &argument = node->arguments[i];
                if (i > 0)
                {
                    result += ", ";
                }
                result += emitExpression(*argument);
            }
            result += ")";
            return result;
        }
        if (const auto *node = dynamic_cast<const ListExpr *>(&expr))
        {
            std::string type = node->elementType;
            std::string cppType = mapType(type);
            std::string size = emitExpression(*node->size);
            std::string result = "gash::List<" + cppType + ">(" + size + ")";
            return result;
        }
        if (const auto *node = dynamic_cast<const IndexExpr *>(&expr))
        {
            std::string collection = emitExpression(*node->collection);
            std::string index = emitExpression(*node->index);
            return "(" + collection + ").get(" + index + ")";
        }
        throw std::runtime_error("Expression type not yet supported by CodeGenerator");
    }

    // Escape a GASH string for C++ output.
    std::string CodeGenerator::escapeString(const std::string &value)
    {
        std::string result;
        for (std::size_t i = 0; i < value.size(); i++)
        {
            if (value[i] == '\\')
            {
                result += "\\\\";
            }
            else if (value[i] == '"')
            {
                result += "\\\"";
            }
            else if (value[i] == '\n')
            {
                result += "\\n";
            }
            else if (value[i] == '\t')
            {
                result += "\\t";
            }
            else if (value[i] == '\r')
            {
                result += "\\r";
            }
            else
            {
                result += value[i];
            }
        }
        return result;
    }

    std::string CodeGenerator::generate(const Program &program)
    {
        indentLevel = 0;
        nextRepeatId = 0;
        nextScopeGuardId = 0;
        functionReturnTypes.clear();
        output.str("");
        output.clear();

        // Populate inferred function return types using SemanticAnalyzer when available.
        try
        {
            SemanticAnalyzer semantic;
            semantic.analyze(program);
            for (const auto &stmt : program.statements)
            {
                if (const auto *fn = dynamic_cast<const FunctionDeclStmt *>(stmt.get()))
                {
                    functionReturnTypes[fn->name] = semantic.getFunctionReturnType(fn->name);
                }
            }
        }
        catch (...)
        {
            // Allow standalone CodeGenerator tests with partial ASTs to proceed using auto deduction.
        }

        emitLine("#include <iostream>");
        emitLine("#include <string>");
        emitLine("#include <cmath>");
        emitLine("#include <stdexcept>");
        emitLine("#include <type_traits>");
        emitLine("#include \"Budget.h\"");
        emitLine("#include \"Checkpoint.h\"");
        emitLine("#include \"Environment.h\"");
        emitLine("#include \"Inspect.h\"");
        emitLine("#include \"List.h\"");
        emitLine("#include \"Preview.h\"");
        emitLine("#include \"Value.h\"");
        emitLine("");
        emitLine("static gash::Environment gash_env;");
        emitLine("static gash::CheckpointManager gash_checkpoints;");
        emitLine("");

        for (const auto &stmt : program.statements)
        {
            if (dynamic_cast<const FunctionDeclStmt *>(stmt.get()))
            {
                emitStatement(*stmt);
                emitLine("");
            }
        }

        emitLine("int main(){");
        indentLevel++;
        emitLine("gash::registerPreviewEnvironment(&gash_env);");

        for (const auto &stmt : program.statements)
        {
            if (!dynamic_cast<const FunctionDeclStmt *>(stmt.get()))
            {
                emitStatement(*stmt);
            }
        }

        emitLine("return 0;");

        indentLevel--;
        emitLine("}");

        return output.str();
    }

    // helper
    std::string CodeGenerator::mapType(const std::string &gashType)
    {
        Type parsed = Type::fromName(gashType);
        if (parsed.isValid() && !parsed.isVoid())
        {
            return parsed.toCppType();
        }

        throw std::runtime_error(
            "Unsupported GASH type: " + gashType);
    }
}

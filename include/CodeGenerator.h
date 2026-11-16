// Declares CodeGenerator, which lowers a validated GASH AST into executable C++17 source code.
#ifndef CODEGENERATOR_H
#define CODEGENERATOR_H

#include <sstream>
#include <string>
#include <unordered_map>
#include <unordered_set>

#include "AST.h"
#include "Type.h"

namespace gash
{
    // Translates a validated GASH AST into a self-contained C++17 source file
    // backed by the GASH runtime (Environment, Guards, Checkpoints, Budgets, Lists).
    class CodeGenerator
    {
    private:
        std::ostringstream output;
        int indentLevel = 0;
        int nextRepeatId = 0;
        int nextScopeGuardId = 0;
        bool inFunctionOrBlock = false;
        std::unordered_map<std::string, Type> functionReturnTypes;
        std::unordered_map<const FunctionDeclStmt *, Type> functionReturnTypesByNode;
        std::unordered_set<std::string> hoistedGlobals;

        // Emits one indented line to the output C++ buffer.
        void emitLine(const std::string &line);
        // Emits C++17 code for a single AST statement node.
        void emitStatement(const Stmt &stmt);
        // Emits a C++17 expression string for an AST expression node.
        std::string emitExpression(const Expr &expr) const;
        // Resolves the inferred return type for a declared function node.
        Type resolveFunctionReturnType(const FunctionDeclStmt &fn) const;
        // Escapes a string literal safely for C++17 source emission.
        static std::string escapeString(const std::string &value);
        // Escapes a single character literal safely for C++17 source emission.
        static std::string escapeChar(char value);
        // Maps a GASH type name string to its corresponding C++17 type.
        static std::string mapType(const std::string &gashType);

    public:
        // Generates complete C++17 source code for the given GASH program AST.
        std::string generate(const Program &program);
    };
}

#endif

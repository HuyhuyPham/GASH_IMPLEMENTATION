#ifndef CODEGENERATOR_H
#define CODEGENERATOR_H

#include <string>
#include <sstream>

#include "AST.h"

namespace gash
{
    class CodeGenerator
    {
    private:
        std::ostringstream output;
        int indentLevel = 0;
        int nextRepeatId = 0;

        void emitLine(const std::string &line);
        void emitStatement(const Stmt &stmt);
        std::string emitExpression(const Expr &expr) const;
        static std::string escapeString(const std::string &value);
        static std::string mapType(const std::string& gashType);

    public:
        std::string generate(const Program &program);
    };
}

#endif
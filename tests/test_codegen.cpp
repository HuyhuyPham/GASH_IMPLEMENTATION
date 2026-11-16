// Smoke test verifying basic C++ code generation for variable declaration and output().
#if __has_include("../include/CodeGenerator.h")
#include "../include/CodeGenerator.h"
#else
#include "CodeGenerator.h"
#endif

#include <iostream>
#include <memory>
#include <utility>
#include <vector>

// Builds a minimal AST and prints generated C++ code to stdout.
int main() {
    using namespace gash;

    Program program;

    // var x = 10;
    program.statements.push_back(
        std::make_unique<VarDeclStmt>(
            "x",
            std::make_unique<IntegerExpr>(10)
        )
    );

    // output(x + 20);
    std::vector<std::unique_ptr<Expr>> args;

    args.push_back(
        std::make_unique<BinaryExpr>(
            "+",
            std::make_unique<VariableExpr>("x"),
            std::make_unique<IntegerExpr>(20)
        )
    );

    program.statements.push_back(
        std::make_unique<ExpressionStmt>(
            std::make_unique<CallExpr>(
                "output",
                std::move(args)
            )
        )
    );

    CodeGenerator generator;

    std::cout << generator.generate(program);
}
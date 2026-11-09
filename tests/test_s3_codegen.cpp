
#include "CodeGenerator.h"

#include <iostream>
#include <memory>
#include <utility>
#include <vector>
#include <exception>

using namespace gash;

int main() {
    try {
        Program program;

        // TEST 1: Create function add(a: int, b: int)
        // func add(a: int, b: int) {
        //     return a + b;
        // }

        std::vector<Parameter> parameters;
        parameters.emplace_back("a", "int");
        parameters.emplace_back("b", "int");

        // Create return expression: a + b
        auto returnExpr = std::make_unique<BinaryExpr>(
            "+",
            std::make_unique<VariableExpr>("a"),
            std::make_unique<VariableExpr>("b")
        );

        // Create return statement
        std::vector<std::unique_ptr<Stmt>> functionBody;

        functionBody.push_back(
            std::make_unique<ReturnStmt>(
                std::move(returnExpr)
            )
        );

        // Create function declaration
        program.statements.push_back(
            std::make_unique<FunctionDeclStmt>(
                "add",
                std::move(parameters),
                std::make_unique<BlockStmt>(
                    std::move(functionBody)
                )
            )
        );

        // TEST 2: Call add(10, 20)
        // var result = add(10, 20);

        std::vector<std::unique_ptr<Expr>> callArgs;

        callArgs.push_back(
            std::make_unique<IntegerExpr>(10)
        );

        callArgs.push_back(
            std::make_unique<IntegerExpr>(20)
        );

        program.statements.push_back(
            std::make_unique<VarDeclStmt>(
                "result",
                std::make_unique<CallExpr>(
                    "add",
                    std::move(callArgs)
                )
            )
        );

        // TEST 3: Print the result
        // output(result);

        std::vector<std::unique_ptr<Expr>> outputArgs;

        outputArgs.push_back(
            std::make_unique<VariableExpr>("result")
        );

        program.statements.push_back(
            std::make_unique<ExpressionStmt>(
                std::make_unique<CallExpr>(
                    "output",
                    std::move(outputArgs)
                )
            )
        );

        // Generate C++ source code
        CodeGenerator generator;

        std::cout << generator.generate(program);

        return 0;

    } catch (const std::exception& error) {
        std::cerr << "Code generation failed: "
                  << error.what() << '\n';
        return 1;
    }
}
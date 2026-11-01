#include "CodeGenerator.h"

#include <iostream>
#include <memory>
#include <utility>
#include <vector>

using namespace gash;

// Helper: create an if (x == number) statement
// containing one action, such as skip or exit.
std::unique_ptr<Stmt> makeIfEquals(
    int number,
    std::unique_ptr<Stmt> action
) {
    auto condition = std::make_unique<BinaryExpr>(
        "==",
        std::make_unique<VariableExpr>("x"),
        std::make_unique<IntegerExpr>(number)
    );

    std::vector<std::unique_ptr<Stmt>> body;
    body.push_back(std::move(action));

    return std::make_unique<IfStmt>(
        std::move(condition),
        std::make_unique<BlockStmt>(std::move(body)),
        nullptr
    );
}

int main() {
    Program program;

    // 1. var x = 0;
    program.statements.push_back(
        std::make_unique<VarDeclStmt>(
            "x",
            std::make_unique<IntegerExpr>(0)
        )
    );

    std::vector<std::unique_ptr<Stmt>> repeatBody;

    // 2. x = x + 1;
    repeatBody.push_back(
        std::make_unique<ExpressionStmt>(
            std::make_unique<AssignmentExpr>(
                std::make_unique<VariableExpr>("x"),
                std::make_unique<BinaryExpr>(
                    "+",
                    std::make_unique<VariableExpr>("x"),
                    std::make_unique<IntegerExpr>(1)
                )
            )
        )
    );

    // 3. if (x == 2) { skip; }
    repeatBody.push_back(
        makeIfEquals(2, std::make_unique<SkipStmt>())
    );

    // 4. if (x == 5) { exit; }
    repeatBody.push_back(
        makeIfEquals(5, std::make_unique<ExitStmt>())
    );

    // 5. output(x);
    std::vector<std::unique_ptr<Expr>> args;
    args.push_back(std::make_unique<VariableExpr>("x"));

    repeatBody.push_back(
        std::make_unique<ExpressionStmt>(
            std::make_unique<CallExpr>(
                "output",
                std::move(args)
            )
        )
    );

    // 6. repeat(6) { ... }
    program.statements.push_back(
        std::make_unique<RepeatStmt>(
            std::make_unique<IntegerExpr>(6),
            std::make_unique<BlockStmt>(
                std::move(repeatBody)
            )
        )
    );

    // Generate C++ from the manually constructed AST.
    CodeGenerator generator;
    std::cout << generator.generate(program);

    return 0;
}
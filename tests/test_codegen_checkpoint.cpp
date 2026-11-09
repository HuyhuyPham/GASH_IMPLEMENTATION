
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include "AST.h"
#include "CodeGenerator.h"

using namespace gash;

int main() {
    Program program;

    // Create a variable declaration.
    program.statements.push_back(
        std::make_unique<VarDeclStmt>(
            "x",
            std::make_unique<IntegerExpr>(10)
        )
    );

    // Create a checkpoint statement.
    program.statements.push_back(
        std::make_unique<CheckpointStmt>(
            "start",
            std::vector<std::string>{"x"}
        )
    );

    // Create a restore statement.
    program.statements.push_back(
        std::make_unique<RestoreStmt>("start")
    );

    CodeGenerator generator;

    try {
        std::string generated = generator.generate(program);
        std::cout << generated << '\n';
    } catch (const std::runtime_error& error) {
        std::cerr << "G7 integration failure: "
                  << error.what() << '\n';
        return 1;
    }

    std::cout << "Checkpoint code generation supported!\n";
    return 0;
}
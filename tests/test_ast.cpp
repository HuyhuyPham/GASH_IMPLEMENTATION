
#include "AST.h"
#include <cassert>
#include <iostream>
#include <memory>
#include <utility>

int main() {
    // Create a program
    gash::Program program;

    // Create integer literal: 10
    auto value = std::make_unique<gash::IntegerExpr>(10);

    // Create variable declaration: var x = 10;
    auto declaration =
        std::make_unique<gash::VarDeclStmt>(
            "x",
            std::move(value)
        );

    // Add declaration to the program
    program.statements.push_back(std::move(declaration));

    // TEST 1: There should be exactly one statement
    assert(program.statements.size() == 1);

    // TEST 2: That statement should be VarDeclStmt
    auto* var = dynamic_cast<gash::VarDeclStmt*>(
        program.statements[0].get()
    );

    assert(var != nullptr);

    // TEST 3: The variable should be named "x"
    assert(var->name == "x");

    // TEST 4: The initializer should contain integer 10
    auto* integer = dynamic_cast<gash::IntegerExpr*>(
        var->initializer.get()
    );

    assert(integer != nullptr);
    assert(integer->value == 10);

    // Display success only if all assertions pass
    std::cout << "AST test passed!" << std::endl;

    return 0;
}


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

    // TEST: There should be exactly one statement
    assert(program.statements.size() == 1);

    // TEST: That statement should be VarDeclStmt
    auto* var = dynamic_cast<gash::VarDeclStmt*>(
        program.statements[0].get()
    );

    assert(var != nullptr);

    // TEST: The variable should be named "x"
    assert(var->name == "x");

    // TEST: Create and check the unary expression -10
    auto number = std::make_unique<gash::IntegerExpr>(10);

    auto unary = std::make_unique<gash::UnaryExpr>(
        "-",
        std::move(number)
    );

    // Check the operator
    assert(unary->op == "-");

    // Check that the operand is an integer literal
    auto* operand = dynamic_cast<gash::IntegerExpr*>(
        unary->operand.get()
    );

    assert(operand != nullptr);
    assert(operand->value == 10);

    // TEST: The initializer should contain integer 10
    auto* integer = dynamic_cast<gash::IntegerExpr*>(
        var->initializer.get()
    );

    assert(integer != nullptr);
    assert(integer->value == 10);

    // TEST: Create and check a floating-point literal
    auto decimal = std::make_unique<gash::FloatExpr>(19.99);

    // Check that the value is stored correctly
    assert(decimal->value > 19.98);
    assert(decimal->value < 20.00);

    // TEST: Create and check a string literal
    auto text = std::make_unique<gash::StringExpr>("Alice");

    // Check that the string value is stored correctly
    assert(text->value == "Alice");

    // TEST: Create and check Boolean literals
    auto booleanTrue = std::make_unique<gash::BoolExpr>(true);
    auto booleanFalse = std::make_unique<gash::BoolExpr>(false);

    // Check both Boolean values
    assert(booleanTrue->value == true);
    assert(booleanFalse->value == false);

    // TEST: Create and check a character literal
    auto character = std::make_unique<gash::CharExpr>('A');

    // Check that the character is stored correctly
    assert(character->value == 'A');

    // Display success only if all assertions pass
    std::cout << "AST test passed!" << std::endl;

    return 0;
}

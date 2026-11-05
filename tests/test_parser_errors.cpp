
// Parser error handling tests.

#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "Lexer.h"
#include "Parser.h"

using namespace gash;

// Return true if the Parser rejects invalid syntax.
bool parseFails(const std::string& source) {
    Lexer lexer(source);
    auto tokens = lexer.tokenize();

    Parser parser(std::move(tokens));

    try {
        auto program = parser.parseProgram();
        (void)program;
        return false;
    } catch (const std::runtime_error&) {
        return true;
    }
}

// Check that invalid GASH syntax is rejected.
void expectInvalid(const std::string& source) {
    if (!parseFails(source)) {
        throw std::runtime_error(
            "Parser accepted invalid syntax: " + source
        );
    }
}

// Check that valid GASH syntax is accepted.
void expectValid(const std::string& source) {
    Lexer lexer(source);
    auto tokens = lexer.tokenize();

    Parser parser(std::move(tokens));
    auto program = parser.parseProgram();
    (void)program;
}

int main() {

    // Test 1: Missing variable value.
    expectInvalid("var x = ;");

    // Test 2: Missing semicolon.
    expectInvalid("var x = 10");

    // Test 3: Missing variable name.
    expectInvalid("var = 10;");

    // Test 4: Missing closing brace.
    expectInvalid("if (true) { var x = 1;");

    // Test 5: Invalid function parameter.
    expectInvalid(
        "func add(int a,) { return a; }"
    );

    // Test 6: Trailing comma in checkpoint.
    expectInvalid("checkpoint start(x,);");

    // Test 7: Missing restore label.
    expectInvalid("restore();");

    // Test 8: Missing budget closing brace.
    expectInvalid("budget(10) { var x = 1;");

    // Test 9: Missing closing parenthesis.
    expectInvalid("var x = (1 + 2;");

    // Test 10: Missing checkpoint variable.
    expectInvalid("checkpoint start();");

    // Test 11: Valid variable declaration.
    expectValid("var x = 10;");

    // Test 12: Valid function declaration.
    expectValid(
        "func add(int a, int b) { return a + b; }"
    );

    // Test 13: Valid checkpoint and restore.
    expectValid(
        "checkpoint start(x); restore(start);"
    );

    std::cout << "Parser error tests passed!\n";

    return 0;
}
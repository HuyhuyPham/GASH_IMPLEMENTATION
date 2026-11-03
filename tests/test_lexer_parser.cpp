
#include "Lexer.h"
#include "Parser.h"

#include <cassert>
#include <iostream>
#include <memory>
#include <string>
#include <utility>

using namespace gash;

// Converts GASH source code into an expression AST.
std::unique_ptr<Expr> parseSource(const std::string& source) {

    // Step 1: Create the Lexer.
    Lexer lexer(source);

    // Step 2: Convert source code into tokens.
    auto tokens = lexer.tokenize();

    // Step 3: Give the tokens to the Parser.
    Parser parser(std::move(tokens));

    // Step 4: Return the expression AST.
    return parser.parseExpressionOnly();
}

int main() {

    // Test 1: Arithmetic precedence.
    {
        auto expr = parseSource("10 + 5 * 2");

        auto* addition =
            dynamic_cast<BinaryExpr*>(expr.get());

        assert(addition != nullptr);
        assert(addition->op == "+");

        auto* multiplication =
            dynamic_cast<BinaryExpr*>(addition->right.get());

        assert(multiplication != nullptr);
        assert(multiplication->op == "*");
    }

    // Test 2: Logical precedence.
    {
        auto expr = parseSource("a || b && c");

        auto* logicalOr =
            dynamic_cast<BinaryExpr*>(expr.get());

        assert(logicalOr != nullptr);
        assert(logicalOr->op == "||");

        auto* logicalAnd =
            dynamic_cast<BinaryExpr*>(logicalOr->right.get());

        assert(logicalAnd != nullptr);
        assert(logicalAnd->op == "&&");
    }

    // Test 3: Function call.
    {
        auto expr = parseSource("add(2, 3)");

        auto* call =
            dynamic_cast<CallExpr*>(expr.get());

        assert(call != nullptr);
        assert(call->callee == "add");
        assert(call->arguments.size() == 2);
    }

    // Test 4: List indexing.
    {
        auto expr = parseSource("numbers[0]");

        auto* index =
            dynamic_cast<IndexExpr*>(expr.get());

        assert(index != nullptr);
    }

    // Test 5: List construction.
    {
        auto expr = parseSource("list<int>(3)");

        auto* list =
            dynamic_cast<ListExpr*>(expr.get());

        assert(list != nullptr);
        assert(list->elementType == "int");
    }

    std::cout << "Lexer-Parser integration tests passed!\n";

    return 0;
}
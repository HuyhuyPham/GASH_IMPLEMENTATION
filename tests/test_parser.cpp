
#include "Parser.h"

#include <cassert>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <utility>
#include <vector>

using namespace gash;

// Parse a token list into an expression AST.
std::unique_ptr<Expr> parse(std::vector<Token> tokens) {
    Parser parser(std::move(tokens));
    return parser.parseExpressionOnly();
}

int main() {

    // Test 1: Arithmetic precedence.
    {
        auto expr = parse({
            {TokenType::INTEGER_LITERAL, "10", 1, 1},
            {TokenType::PLUS, "+", 1, 4},
            {TokenType::INTEGER_LITERAL, "5", 1, 6},
            {TokenType::STAR, "*", 1, 8},
            {TokenType::INTEGER_LITERAL, "2", 1, 10}
        });

        auto* addition = dynamic_cast<BinaryExpr*>(expr.get());

        assert(addition != nullptr);
        assert(addition->op == "+");

        auto* multiplication =
            dynamic_cast<BinaryExpr*>(addition->right.get());

        assert(multiplication != nullptr);
        assert(multiplication->op == "*");
    }

    // Test 2: Power is right-associative.
    {
        auto expr = parse({
            {TokenType::INTEGER_LITERAL, "2", 1, 1},
            {TokenType::CARET, "^", 1, 3},
            {TokenType::INTEGER_LITERAL, "3", 1, 5},
            {TokenType::CARET, "^", 1, 7},
            {TokenType::INTEGER_LITERAL, "2", 1, 9}
        });

        auto* outer = dynamic_cast<BinaryExpr*>(expr.get());

        assert(outer != nullptr);
        assert(outer->op == "^");

        auto* inner =
            dynamic_cast<BinaryExpr*>(outer->right.get());

        assert(inner != nullptr);
        assert(inner->op == "^");
    }

    // Test 3: Unary minus and exponentiation.
    {
        auto expr = parse({
            {TokenType::MINUS, "-", 1, 1},
            {TokenType::INTEGER_LITERAL, "2", 1, 2},
            {TokenType::CARET, "^", 1, 4},
            {TokenType::INTEGER_LITERAL, "2", 1, 6}
        });

        auto* unary = dynamic_cast<UnaryExpr*>(expr.get());

        assert(unary != nullptr);
        assert(unary->op == "-");

        auto* power =
            dynamic_cast<BinaryExpr*>(unary->operand.get());

        assert(power != nullptr);
        assert(power->op == "^");
    }

    // Test 4: Logical AND has higher precedence than OR.
    {
        auto expr = parse({
            {TokenType::IDENTIFIER, "a", 1, 1},
            {TokenType::OR_OR, "||", 1, 3},
            {TokenType::IDENTIFIER, "b", 1, 6},
            {TokenType::AND_AND, "&&", 1, 8},
            {TokenType::IDENTIFIER, "c", 1, 11}
        });

        auto* logicalOr = dynamic_cast<BinaryExpr*>(expr.get());

        assert(logicalOr != nullptr);
        assert(logicalOr->op == "||");

        auto* logicalAnd =
            dynamic_cast<BinaryExpr*>(logicalOr->right.get());

        assert(logicalAnd != nullptr);
        assert(logicalAnd->op == "&&");
    }

    // Test 5: Assignment is right-associative.
    {
        auto expr = parse({
            {TokenType::IDENTIFIER, "a", 1, 1},
            {TokenType::ASSIGN, "=", 1, 3},
            {TokenType::IDENTIFIER, "b", 1, 5},
            {TokenType::ASSIGN, "=", 1, 7},
            {TokenType::INTEGER_LITERAL, "10", 1, 9}
        });

        auto* outer =
            dynamic_cast<AssignmentExpr*>(expr.get());

        assert(outer != nullptr);

        auto* inner =
            dynamic_cast<AssignmentExpr*>(outer->value.get());

        assert(inner != nullptr);
    }

    // Test 6: Function calls.
    {
        auto expr = parse({
            {TokenType::IDENTIFIER, "add", 1, 1},
            {TokenType::LEFT_PAREN, "(", 1, 4},
            {TokenType::INTEGER_LITERAL, "2", 1, 5},
            {TokenType::COMMA, ",", 1, 6},
            {TokenType::INTEGER_LITERAL, "3", 1, 8},
            {TokenType::RIGHT_PAREN, ")", 1, 9}
        });

        auto* call = dynamic_cast<CallExpr*>(expr.get());

        assert(call != nullptr);
        assert(call->callee == "add");
        assert(call->arguments.size() == 2);
    }

    // Test 7: List indexing.
    {
        auto expr = parse({
            {TokenType::IDENTIFIER, "numbers", 1, 1},
            {TokenType::LEFT_BRACKET, "[", 1, 8},
            {TokenType::INTEGER_LITERAL, "0", 1, 9},
            {TokenType::RIGHT_BRACKET, "]", 1, 10}
        });

        auto* index = dynamic_cast<IndexExpr*>(expr.get());

        assert(index != nullptr);

        auto* variable =
            dynamic_cast<VariableExpr*>(index->collection.get());

        assert(variable != nullptr);
        assert(variable->name == "numbers");
    }

    // Test 8: List construction.
    {
        auto expr = parse({
            {TokenType::TYPE_LIST, "list", 1, 1},
            {TokenType::LESS, "<", 1, 5},
            {TokenType::TYPE_INT, "int", 1, 6},
            {TokenType::GREATER, ">", 1, 9},
            {TokenType::LEFT_PAREN, "(", 1, 10},
            {TokenType::INTEGER_LITERAL, "3", 1, 11},
            {TokenType::RIGHT_PAREN, ")", 1, 12}
        });

        auto* list = dynamic_cast<ListExpr*>(expr.get());

        assert(list != nullptr);
        assert(list->elementType == "int");

        auto* size =
            dynamic_cast<IntegerExpr*>(list->size.get());

        assert(size != nullptr);
        assert(size->value == 3);
    }

    // Test 9: Invalid assignment target.
    {
        bool rejected = false;

        try {
            parse({
                {TokenType::INTEGER_LITERAL, "10", 1, 1},
                {TokenType::ASSIGN, "=", 1, 4},
                {TokenType::INTEGER_LITERAL, "5", 1, 6}
            });
        }
        catch (const std::runtime_error&) {
            rejected = true;
        }

        assert(rejected);
    }

    std::cout << "Parser expression tests passed!\n";

    return 0;
}

#include <cassert>
#include <iostream>
#include <memory>
#include <string>
#include <utility>

#if __has_include("../include/Lexer.h")
#include "../include/Lexer.h"
#include "../include/Parser.h"
#else
#include "Lexer.h"
#include "Parser.h"
#endif

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

    auto* addition = dynamic_cast<BinaryExpr*>(expr.get());

    assert(addition != nullptr);
    assert(addition->op == "+");

    auto* multiplication = dynamic_cast<BinaryExpr*>(addition->right.get());

    assert(multiplication != nullptr);
    assert(multiplication->op == "*");
  }

  // Test 2: Logical precedence.
  {
    auto expr = parseSource("a || b && c");

    auto* logicalOr = dynamic_cast<BinaryExpr*>(expr.get());

    assert(logicalOr != nullptr);
    assert(logicalOr->op == "||");

    auto* logicalAnd = dynamic_cast<BinaryExpr*>(logicalOr->right.get());

    assert(logicalAnd != nullptr);
    assert(logicalAnd->op == "&&");
  }

  // Test 3: Function call.
  {
    auto expr = parseSource("add(2, 3)");

    auto* call = dynamic_cast<CallExpr*>(expr.get());

    assert(call != nullptr);
    assert(call->callee == "add");
    assert(call->arguments.size() == 2);
  }

  // Test 4: List indexing.
  {
    auto expr = parseSource("numbers[0]");

    auto* index = dynamic_cast<IndexExpr*>(expr.get());

    assert(index != nullptr);
  }

  // Test 5: List construction.
  {
    auto expr = parseSource("list<int>(3)");

    auto* list = dynamic_cast<ListExpr*>(expr.get());

    assert(list != nullptr);
    assert(list->elementType == "int");
  }

  // Test 6: String escape sequence.
  {
    auto expr = parseSource("\"Hello\\nWorld\"");

    auto* str = dynamic_cast<StringExpr*>(expr.get());

    assert(str != nullptr);
    assert(str->value == "Hello\nWorld");
  }

  // Test 7: Character escape sequence.
  {
    auto expr = parseSource("'\\n'");

    auto* character = dynamic_cast<CharExpr*>(expr.get());

    assert(character != nullptr);
    assert(character->value == '\n');
  }

  // Test 8: AST source locations.
  {
    // The expression starts on line 2, column 3.
    auto expr = parseSource("\n  x + 10");

    // Check the binary expression.
    auto* addition = dynamic_cast<BinaryExpr*>(expr.get());

    assert(addition != nullptr);
    assert(addition->location.line == 2);
    assert(addition->location.column == 3);

    // Check the left variable.
    auto* variable = dynamic_cast<VariableExpr*>(addition->left.get());

    assert(variable != nullptr);
    assert(variable->location.line == 2);
    assert(variable->location.column == 3);

    // Check the right integer.
    auto* number = dynamic_cast<IntegerExpr*>(addition->right.get());

    assert(number != nullptr);
    assert(number->location.line == 2);
    assert(number->location.column == 7);
  }

  // Test 9: Logical expression source locations.
  {
    auto expr = parseSource("\n  a || b && c");

    // Check logical OR location.
    auto* logicalOr = dynamic_cast<BinaryExpr*>(expr.get());

    assert(logicalOr != nullptr);
    assert(logicalOr->location.line == 2);
    assert(logicalOr->location.column == 3);

    // Check logical AND location.
    auto* logicalAnd = dynamic_cast<BinaryExpr*>(logicalOr->right.get());

    assert(logicalAnd != nullptr);
    assert(logicalAnd->location.line == 2);
    assert(logicalAnd->location.column == 8);
  }

  std::cout << "Lexer-Parser integration tests passed!\n";

  return 0;
}
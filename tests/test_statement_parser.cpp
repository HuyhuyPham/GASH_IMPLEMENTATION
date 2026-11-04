
#include <cassert>
#include <iostream>
#include <utility>

#include "Lexer.h"
#include "Parser.h"

using namespace gash;

int main() {
  // Two complete GASH statements.
  Lexer lexer("x = 10; y = x + 5;");

  // Convert source code into tokens.
  auto tokens = lexer.tokenize();

  // Create the Parser.
  Parser parser(std::move(tokens));

  // Parse the complete program.
  Program program = parser.parseProgram();

  // The program should contain two statements.
  assert(program.statements.size() == 2);

  // Check the first statement.
  auto* first = dynamic_cast<ExpressionStmt*>(program.statements[0].get());

  assert(first != nullptr);

  // Check that it contains an assignment.
  auto* assignment = dynamic_cast<AssignmentExpr*>(first->expression.get());

  assert(assignment != nullptr);

  // Check the second statement.
  auto* second = dynamic_cast<ExpressionStmt*>(program.statements[1].get());

  assert(second != nullptr);

  // Test 2: Variable declarations.
  {
    // Create a GASH program with three variables.
    Lexer lexer(
        "var x = 10;"
        "var name = \"Alice\";"
        "var numbers = list<int>(5);");

    // Convert source code into tokens.
    auto tokens = lexer.tokenize();

    // Parse the complete program.
    Parser parser(std::move(tokens));
    Program program = parser.parseProgram();

    // Check the number of statements.
    assert(program.statements.size() == 3);

    // Check the first variable declaration.
    auto* first = dynamic_cast<VarDeclStmt*>(program.statements[0].get());

    assert(first != nullptr);
    assert(first->name == "x");

    // Check the initial integer value.
    auto* integer = dynamic_cast<IntegerExpr*>(first->initializer.get());

    assert(integer != nullptr);
    assert(integer->value == 10);

    // Check the second variable declaration.
    auto* second = dynamic_cast<VarDeclStmt*>(program.statements[1].get());

    assert(second != nullptr);
    assert(second->name == "name");

    // Check the third variable declaration.
    auto* third = dynamic_cast<VarDeclStmt*>(program.statements[2].get());

    assert(third != nullptr);
    assert(third->name == "numbers");

    // Check the list initializer.
    auto* list = dynamic_cast<ListExpr*>(third->initializer.get());

    assert(list != nullptr);
    assert(list->elementType == "int");
  }

  // Test 3: Block statements.
  {
    Lexer lexer(
        "{"
        "  var x = 10;"
        "  x = x + 5;"
        "}");

    auto tokens = lexer.tokenize();

    Parser parser(std::move(tokens));
    Program program = parser.parseProgram();

    // Check that the program contains one block.
    assert(program.statements.size() == 1);

    auto* block = dynamic_cast<BlockStmt*>(program.statements[0].get());

    assert(block != nullptr);

    // The block should contain two statements.
    assert(block->statements.size() == 2);

    // First statement: variable declaration.
    auto* declaration = dynamic_cast<VarDeclStmt*>(block->statements[0].get());

    assert(declaration != nullptr);
    assert(declaration->name == "x");

    // Second statement: assignment expression.
    auto* expression =
        dynamic_cast<ExpressionStmt*>(block->statements[1].get());

    assert(expression != nullptr);
  }

  // Test 4: Nested and empty blocks.
  {
    Lexer lexer("{ {} }");

    auto tokens = lexer.tokenize();

    Parser parser(std::move(tokens));
    Program program = parser.parseProgram();

    // Check the outer block.
    assert(program.statements.size() == 1);

    auto* outer = dynamic_cast<BlockStmt*>(program.statements[0].get());

    assert(outer != nullptr);
    assert(outer->statements.size() == 1);

    // Check the inner block.
    auto* inner = dynamic_cast<BlockStmt*>(outer->statements[0].get());

    assert(inner != nullptr);
    assert(inner->statements.empty());
  }

  // Test 5: Return statements.
  {
    // Create a block containing a return statement.
    Lexer lexer("{ var x = 10; return x + 5; }");

    auto tokens = lexer.tokenize();

    Parser parser(std::move(tokens));
    Program program = parser.parseProgram();

    // Check the outer block.
    assert(program.statements.size() == 1);

    auto* block = dynamic_cast<BlockStmt*>(program.statements[0].get());

    assert(block != nullptr);
    assert(block->statements.size() == 2);

    // Check the return statement.
    auto* returnStmt = dynamic_cast<ReturnStmt*>(block->statements[1].get());

    assert(returnStmt != nullptr);

    // Check the returned expression.
    auto* addition = dynamic_cast<BinaryExpr*>(returnStmt->value.get());

    assert(addition != nullptr);
    assert(addition->op == "+");

    // Check the return statement's location.
    assert(returnStmt->location.line == 1);
    assert(returnStmt->location.column == 15);
  }

  std::cout << "Basic statement parser tests passed!\n";

  return 0;
}
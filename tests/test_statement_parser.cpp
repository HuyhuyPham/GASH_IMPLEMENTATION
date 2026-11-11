
#include <cassert>
#include <iostream>
#include <utility>

#if __has_include("../include/Lexer.h")
#include "../include/Lexer.h"
#include "../include/Parser.h"
#else
#include "Lexer.h"
#include "Parser.h"
#endif

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

  // Test 6: If with otherwise.
  {
    Lexer lexer(
        "if (x > 10) {"
        "  output(x);"
        "} otherwise {"
        "  output(0);"
        "}");

    auto tokens = lexer.tokenize();

    Parser parser(std::move(tokens));
    Program program = parser.parseProgram();

    // The program contains one if statement.
    assert(program.statements.size() == 1);

    auto* ifStmt = dynamic_cast<IfStmt*>(program.statements[0].get());

    assert(ifStmt != nullptr);

    // Check the condition.
    auto* condition = dynamic_cast<BinaryExpr*>(ifStmt->condition.get());

    assert(condition != nullptr);
    assert(condition->op == ">");

    // Check the if block.
    assert(ifStmt->thenBranch != nullptr);
    assert(ifStmt->thenBranch->statements.size() == 1);

    // Check the otherwise block.
    assert(ifStmt->elseBranch != nullptr);
    assert(ifStmt->elseBranch->statements.size() == 1);

    // Check the if statement's location.
    assert(ifStmt->location.line == 1);
    assert(ifStmt->location.column == 1);
  }

  // Test 7: If without otherwise.
  {
    Lexer lexer("if (x > 5) { output(x); }");

    auto tokens = lexer.tokenize();

    Parser parser(std::move(tokens));
    Program program = parser.parseProgram();

    assert(program.statements.size() == 1);

    auto* ifStmt = dynamic_cast<IfStmt*>(program.statements[0].get());

    assert(ifStmt != nullptr);

    // The if block exists.
    assert(ifStmt->thenBranch != nullptr);

    // No otherwise block was provided.
    assert(ifStmt->elseBranch == nullptr);
  }

  // Test 8: While loop.
  {
    Lexer lexer(
        "while (x < 10) {"
        "  x = x + 1;"
        "}");

    auto tokens = lexer.tokenize();

    Parser parser(std::move(tokens));
    Program program = parser.parseProgram();

    // Check the program contains one while statement.
    assert(program.statements.size() == 1);

    auto* whileStmt = dynamic_cast<WhileStmt*>(program.statements[0].get());

    assert(whileStmt != nullptr);

    // Check the loop condition.
    auto* condition = dynamic_cast<BinaryExpr*>(whileStmt->condition.get());

    assert(condition != nullptr);
    assert(condition->op == "<");

    // Check the loop body.
    assert(whileStmt->body != nullptr);
    assert(whileStmt->body->statements.size() == 1);

    // Check the assignment inside the loop.
    auto* statement =
        dynamic_cast<ExpressionStmt*>(whileStmt->body->statements[0].get());

    assert(statement != nullptr);

    auto* assignment =
        dynamic_cast<AssignmentExpr*>(statement->expression.get());

    assert(assignment != nullptr);

    // Check the source location.
    assert(whileStmt->location.line == 1);
    assert(whileStmt->location.column == 1);
  }

  // Test 9: While loop with an empty body.
  {
    Lexer lexer("while (true) {}");

    auto tokens = lexer.tokenize();

    Parser parser(std::move(tokens));
    Program program = parser.parseProgram();

    assert(program.statements.size() == 1);

    auto* whileStmt = dynamic_cast<WhileStmt*>(program.statements[0].get());

    assert(whileStmt != nullptr);

    // Check the Boolean condition.
    auto* condition = dynamic_cast<BoolExpr*>(whileStmt->condition.get());

    assert(condition != nullptr);
    assert(condition->value == true);

    // Check the empty block.
    assert(whileStmt->body != nullptr);
    assert(whileStmt->body->statements.empty());
  }

  // Test 10: Repeat loop with an expression count.
  {
    Lexer lexer(
        "repeat (2 + 3) {"
        "  output(\"Hello\");"
        "}");

    auto tokens = lexer.tokenize();

    Parser parser(std::move(tokens));
    Program program = parser.parseProgram();

    // Check the program contains one repeat statement.
    assert(program.statements.size() == 1);

    auto* repeatStmt = dynamic_cast<RepeatStmt*>(program.statements[0].get());

    assert(repeatStmt != nullptr);

    // Check the repeat count expression.
    auto* count = dynamic_cast<BinaryExpr*>(repeatStmt->count.get());

    assert(count != nullptr);
    assert(count->op == "+");

    // Check the loop body.
    assert(repeatStmt->body != nullptr);
    assert(repeatStmt->body->statements.size() == 1);

    // Check the statement inside the loop.
    auto* statement =
        dynamic_cast<ExpressionStmt*>(repeatStmt->body->statements[0].get());

    assert(statement != nullptr);

    // Check the output function call.
    auto* call = dynamic_cast<CallExpr*>(statement->expression.get());

    assert(call != nullptr);
    assert(call->callee == "output");
    assert(call->arguments.size() == 1);

    // Check the source location.
    assert(repeatStmt->location.line == 1);
    assert(repeatStmt->location.column == 1);
  }

  // Test 11: Repeat loop with an empty body.
  {
    Lexer lexer("repeat (5) {}");

    auto tokens = lexer.tokenize();

    Parser parser(std::move(tokens));
    Program program = parser.parseProgram();

    assert(program.statements.size() == 1);

    auto* repeatStmt = dynamic_cast<RepeatStmt*>(program.statements[0].get());

    assert(repeatStmt != nullptr);

    // Check the repeat count.
    auto* count = dynamic_cast<IntegerExpr*>(repeatStmt->count.get());

    assert(count != nullptr);
    assert(count->value == 5);

    // Check the empty body.
    assert(repeatStmt->body != nullptr);
    assert(repeatStmt->body->statements.empty());
  }

  // Test 12: Exit statement inside a while loop.
  {
    Lexer lexer("while (true) { exit; }");

    auto tokens = lexer.tokenize();

    Parser parser(std::move(tokens));
    Program program = parser.parseProgram();

    // Check the while statement.
    assert(program.statements.size() == 1);

    auto* whileStmt = dynamic_cast<WhileStmt*>(program.statements[0].get());

    assert(whileStmt != nullptr);

    // Check the loop body.
    assert(whileStmt->body != nullptr);
    assert(whileStmt->body->statements.size() == 1);

    // Check the exit statement.
    auto* exitStmt =
        dynamic_cast<ExitStmt*>(whileStmt->body->statements[0].get());

    assert(exitStmt != nullptr);

    // Check the source location.
    assert(exitStmt->location.line == 1);
    assert(exitStmt->location.column == 16);
  }

  // Test 13: Skip statement inside a repeat loop.
  {
    Lexer lexer("repeat (5) { skip; }");

    auto tokens = lexer.tokenize();

    Parser parser(std::move(tokens));
    Program program = parser.parseProgram();

    // Check the repeat statement.
    assert(program.statements.size() == 1);

    auto* repeatStmt = dynamic_cast<RepeatStmt*>(program.statements[0].get());

    assert(repeatStmt != nullptr);

    // Check the loop body.
    assert(repeatStmt->body != nullptr);
    assert(repeatStmt->body->statements.size() == 1);

    // Check the skip statement.
    auto* skipStmt =
        dynamic_cast<SkipStmt*>(repeatStmt->body->statements[0].get());

    assert(skipStmt != nullptr);

    // Check the source location.
    assert(skipStmt->location.line == 1);
    assert(skipStmt->location.column == 14);
  }

  // Test 14: Function declaration with parameters.
  {
    Lexer lexer(
        "func add(int a, int b) {"
        "  return a + b;"
        "}");

    auto tokens = lexer.tokenize();

    Parser parser(std::move(tokens));
    Program program = parser.parseProgram();

    // Check the number of statements.
    assert(program.statements.size() == 1);

    // Check the function declaration.
    auto* function =
        dynamic_cast<FunctionDeclStmt*>(program.statements[0].get());

    assert(function != nullptr);
    assert(function->name == "add");

    // Check the parameters.
    assert(function->parameters.size() == 2);
    assert(function->parameters[0].name == "a");
    assert(function->parameters[1].name == "b");
    assert(function->parameters[0].type == "int");
    assert(function->parameters[1].type == "int");

    // Check the function body.
    assert(function->body != nullptr);
    assert(function->body->statements.size() == 1);

    // Check the return statement.
    auto* returnStmt =
        dynamic_cast<ReturnStmt*>(function->body->statements[0].get());

    assert(returnStmt != nullptr);

    // Check the returned expression.
    auto* addition = dynamic_cast<BinaryExpr*>(returnStmt->value.get());

    assert(addition != nullptr);
    assert(addition->op == "+");

    // Check the function's source location.
    assert(function->location.line == 1);
    assert(function->location.column == 1);
  }

  // Test 15: Function without parameters.
  {
    Lexer lexer(
        "func hello() {"
        "  output(\"Hello\");"
        "}");

    auto tokens = lexer.tokenize();

    Parser parser(std::move(tokens));
    Program program = parser.parseProgram();

    assert(program.statements.size() == 1);

    auto* function =
        dynamic_cast<FunctionDeclStmt*>(program.statements[0].get());

    assert(function != nullptr);
    assert(function->name == "hello");

    // Check the empty parameter list.
    assert(function->parameters.empty());

    // Check the function body.
    assert(function->body != nullptr);
    assert(function->body->statements.size() == 1);
  }

  // Test 16: Guard statement with a comparison.
  {
    Lexer lexer("guard x > 0;");

    auto tokens = lexer.tokenize();

    Parser parser(std::move(tokens));
    Program program = parser.parseProgram();

    // Check the program contains one guard statement.
    assert(program.statements.size() == 1);

    auto* guardStmt = dynamic_cast<GuardStmt*>(program.statements[0].get());

    assert(guardStmt != nullptr);

    // Check the guard condition.
    auto* condition = dynamic_cast<BinaryExpr*>(guardStmt->condition.get());

    assert(condition != nullptr);
    assert(condition->op == ">");

    // Check the left variable.
    auto* variable = dynamic_cast<VariableExpr*>(condition->left.get());

    assert(variable != nullptr);
    assert(variable->name == "x");

    // Check the right integer.
    auto* number = dynamic_cast<IntegerExpr*>(condition->right.get());

    assert(number != nullptr);
    assert(number->value == 0);

    // Check the source location.
    assert(guardStmt->location.line == 1);
    assert(guardStmt->location.column == 1);
  }

  // Test 17: Guard statement with a Boolean condition.
  {
    Lexer lexer("guard true;");

    auto tokens = lexer.tokenize();

    Parser parser(std::move(tokens));
    Program program = parser.parseProgram();

    assert(program.statements.size() == 1);

    auto* guardStmt = dynamic_cast<GuardStmt*>(program.statements[0].get());

    assert(guardStmt != nullptr);

    // Check the Boolean condition.
    auto* condition = dynamic_cast<BoolExpr*>(guardStmt->condition.get());

    assert(condition != nullptr);
    assert(condition->value == true);
  }

  // Test 18: Lock and unlock statements.
  {
    Lexer lexer("lock balance; unlock balance;");

    auto tokens = lexer.tokenize();

    Parser parser(std::move(tokens));
    Program program = parser.parseProgram();

    // Check the number of statements.
    assert(program.statements.size() == 2);

    // Check the lock statement.
    auto* lockStmt = dynamic_cast<LockStmt*>(program.statements[0].get());

    assert(lockStmt != nullptr);
    assert(lockStmt->name == "balance");

    // Check the lock source location.
    assert(lockStmt->location.line == 1);
    assert(lockStmt->location.column == 1);

    // Check the unlock statement.
    auto* unlockStmt = dynamic_cast<UnlockStmt*>(program.statements[1].get());

    assert(unlockStmt != nullptr);
    assert(unlockStmt->name == "balance");

    // Check the unlock source location.
    assert(unlockStmt->location.line == 1);
    assert(unlockStmt->location.column == 15);
  }

  // Test 19: Lock and unlock inside a block.
  {
    Lexer lexer(
        "{"
        "  var x = 10;"
        "  lock x;"
        "  unlock x;"
        "}");

    auto tokens = lexer.tokenize();

    Parser parser(std::move(tokens));
    Program program = parser.parseProgram();

    // Check the outer block.
    assert(program.statements.size() == 1);

    auto* block = dynamic_cast<BlockStmt*>(program.statements[0].get());

    assert(block != nullptr);
    assert(block->statements.size() == 3);

    // First statement: variable declaration.
    auto* declaration = dynamic_cast<VarDeclStmt*>(block->statements[0].get());

    assert(declaration != nullptr);

    // Second statement: lock.
    auto* lockStmt = dynamic_cast<LockStmt*>(block->statements[1].get());

    assert(lockStmt != nullptr);
    assert(lockStmt->name == "x");

    // Third statement: unlock.
    auto* unlockStmt = dynamic_cast<UnlockStmt*>(block->statements[2].get());

    assert(unlockStmt != nullptr);
    assert(unlockStmt->name == "x");
  }

  // Test 20: Checkpoint and restore statements.
  {
    Lexer lexer(
        "checkpoint start(lives, score);"
        "restore(start);");

    auto tokens = lexer.tokenize();

    Parser parser(std::move(tokens));
    Program program = parser.parseProgram();

    // Check the number of statements.
    assert(program.statements.size() == 2);

    // Check the checkpoint statement.
    auto* checkpoint =
        dynamic_cast<CheckpointStmt*>(program.statements[0].get());

    assert(checkpoint != nullptr);
    assert(checkpoint->label == "start");

    // Check the checkpoint variables.
    assert(checkpoint->variables.size() == 2);
    assert(checkpoint->variables[0] == "lives");
    assert(checkpoint->variables[1] == "score");

    // Check the source location.
    assert(checkpoint->location.line == 1);
    assert(checkpoint->location.column == 1);

    // Check the restore statement.
    auto* restore = dynamic_cast<RestoreStmt*>(program.statements[1].get());

    assert(restore != nullptr);
    assert(restore->label == "start");
  }

  // Test 21: Invalid checkpoint syntax.
  {
    bool rejected = false;

    try {
      Lexer lexer("checkpoint start(lives,);");

      auto tokens = lexer.tokenize();

      Parser parser(std::move(tokens));
      (void)parser.parseProgram();
    } catch (const std::runtime_error&) {
      rejected = true;
    }

    // The parser must reject the trailing comma.
    assert(rejected);
  }

  // Test 22: Budget statement with a fixed limit.
  {
    Lexer lexer(
        "budget (100) {"
        "  var x = 10;"
        "  output(x);"
        "}");

    auto tokens = lexer.tokenize();

    Parser parser(std::move(tokens));
    Program program = parser.parseProgram();

    // Check the program contains one budget statement.
    assert(program.statements.size() == 1);

    auto* budgetStmt = dynamic_cast<BudgetStmt*>(program.statements[0].get());

    assert(budgetStmt != nullptr);

    // Check the budget limit.
    auto* limit = dynamic_cast<IntegerExpr*>(budgetStmt->limit.get());

    assert(limit != nullptr);
    assert(limit->value == 100);

    // Check the budget block.
    assert(budgetStmt->body != nullptr);
    assert(budgetStmt->body->statements.size() == 2);

    // Check the first statement inside the block.
    auto* declaration =
        dynamic_cast<VarDeclStmt*>(budgetStmt->body->statements[0].get());

    assert(declaration != nullptr);
    assert(declaration->name == "x");

    // Check the source location.
    assert(budgetStmt->location.line == 1);
    assert(budgetStmt->location.column == 1);
  }

  // Test 23: Budget with an expression limit.
  {
    Lexer lexer("budget (2 + 3) {}");

    auto tokens = lexer.tokenize();

    Parser parser(std::move(tokens));
    Program program = parser.parseProgram();

    assert(program.statements.size() == 1);

    auto* budgetStmt = dynamic_cast<BudgetStmt*>(program.statements[0].get());

    assert(budgetStmt != nullptr);

    // Check the budget expression.
    auto* limit = dynamic_cast<BinaryExpr*>(budgetStmt->limit.get());

    assert(limit != nullptr);
    assert(limit->op == "+");

    // Check the empty budget block.
    assert(budgetStmt->body != nullptr);
    assert(budgetStmt->body->statements.empty());
  }

  // Test 24: Explicit type conversion calls.
  {
    Lexer lexer(
        "var age = int(input());"
        "var price = float(\"3.14\");"
        "var text = string(100);");

    auto tokens = lexer.tokenize();

    Parser parser(std::move(tokens));
    Program program = parser.parseProgram();

    // Check the number of statements.
    assert(program.statements.size() == 3);

    // Check int(input()).
    auto* age = dynamic_cast<VarDeclStmt*>(program.statements[0].get());

    assert(age != nullptr);

    auto* intCall = dynamic_cast<CallExpr*>(age->initializer.get());

    assert(intCall != nullptr);
    assert(intCall->callee == "int");
    assert(intCall->arguments.size() == 1);

    // Check the nested input() call.
    auto* inputCall = dynamic_cast<CallExpr*>(intCall->arguments[0].get());

    assert(inputCall != nullptr);
    assert(inputCall->callee == "input");
    assert(inputCall->arguments.empty());

    // Check float("3.14").
    auto* price = dynamic_cast<VarDeclStmt*>(program.statements[1].get());

    assert(price != nullptr);

    auto* floatCall = dynamic_cast<CallExpr*>(price->initializer.get());

    assert(floatCall != nullptr);
    assert(floatCall->callee == "float");
    assert(floatCall->arguments.size() == 1);

    // Check string(100).
    auto* text = dynamic_cast<VarDeclStmt*>(program.statements[2].get());

    assert(text != nullptr);

    auto* stringCall = dynamic_cast<CallExpr*>(text->initializer.get());

    assert(stringCall != nullptr);
    assert(stringCall->callee == "string");
    assert(stringCall->arguments.size() == 1);
  }

  std::cout << "Basic statement parser tests passed!\n";

  return 0;
}
#include "AST.h"
#include <cassert>
#include <iostream>
#include <memory>
#include <utility>
#include <vector>

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

    // TEST: Create a function call add(2, 3)
    std::vector<std::unique_ptr<gash::Expr>> args;

    // Add the first argument: 2
    args.push_back(
        std::make_unique<gash::IntegerExpr>(2)
    );

    // Add the second argument: 3
    args.push_back(
        std::make_unique<gash::IntegerExpr>(3)
    );

    // Create the function call node
    auto call = std::make_unique<gash::CallExpr>(
        "add",
        std::move(args)
    );

    // Check the function name
    assert(call->callee == "add");

    // Check the number of arguments
    assert(call->arguments.size() == 2);

    // Check the first argument
    auto* first = dynamic_cast<gash::IntegerExpr*>(
        call->arguments[0].get()
    );

    assert(first != nullptr);
    assert(first->value == 2);

    // Check the second argument
    auto* second = dynamic_cast<gash::IntegerExpr*>(
        call->arguments[1].get()
    );

    assert(second != nullptr);
    assert(second->value == 3);

    // TEST: Create a block containing two variable declarations
    std::vector<std::unique_ptr<gash::Stmt>> blockStatements;

    // First statement: var x = 10;
    blockStatements.push_back(
        std::make_unique<gash::VarDeclStmt>(
            "x",
            std::make_unique<gash::IntegerExpr>(10)
        )
    );

    // Second statement: var y = 20;
    blockStatements.push_back(
        std::make_unique<gash::VarDeclStmt>(
            "y",
            std::make_unique<gash::IntegerExpr>(20)
        )
    );

    // Create the block node
    auto block = std::make_unique<gash::BlockStmt>(
        std::move(blockStatements)
    );

    // Check that the block contains two statements
    assert(block->statements.size() == 2);

    // Check that the first statement is a variable declaration
    auto* firstStmt = dynamic_cast<gash::VarDeclStmt*>(
        block->statements[0].get()
    );

    assert(firstStmt != nullptr);
    assert(firstStmt->name == "x");

    // Check the initializer value
    auto* firstValue = dynamic_cast<gash::IntegerExpr*>(
        firstStmt->initializer.get()
    );

    assert(firstValue != nullptr);
    assert(firstValue->value == 10);

    // TEST: Create an if statement
    // if (true) { var x = 10; } otherwise { var x = 20; }

    // Create the condition
    auto condition = std::make_unique<gash::BoolExpr>(true);

    // Create the then block
    std::vector<std::unique_ptr<gash::Stmt>> thenStatements;

    thenStatements.push_back(
        std::make_unique<gash::VarDeclStmt>(
            "x",
            std::make_unique<gash::IntegerExpr>(10)
        )
    );

    auto thenBlock = std::make_unique<gash::BlockStmt>(
        std::move(thenStatements)
    );

    // Create the otherwise block
    std::vector<std::unique_ptr<gash::Stmt>> elseStatements;

    elseStatements.push_back(
        std::make_unique<gash::VarDeclStmt>(
            "x",
            std::make_unique<gash::IntegerExpr>(20)
        )
    );

    auto elseBlock = std::make_unique<gash::BlockStmt>(
        std::move(elseStatements)
    );

    // Create the complete if statement
    auto ifStatement = std::make_unique<gash::IfStmt>(
        std::move(condition),
        std::move(thenBlock),
        std::move(elseBlock)
    );

    // Check that all three parts exist
    assert(ifStatement->condition != nullptr);
    assert(ifStatement->thenBranch != nullptr);
    assert(ifStatement->elseBranch != nullptr);

    // Check the condition
    auto* boolCondition = dynamic_cast<gash::BoolExpr*>(
        ifStatement->condition.get()
    );

    assert(boolCondition != nullptr);
    assert(boolCondition->value == true);

    // Check each branch contains one statement
    assert(ifStatement->thenBranch->statements.size() == 1);
    assert(ifStatement->elseBranch->statements.size() == 1);

    // TEST: Create an if statement without otherwise
    std::vector<std::unique_ptr<gash::Stmt>> simpleStatements;

    simpleStatements.push_back(
        std::make_unique<gash::VarDeclStmt>(
            "y",
            std::make_unique<gash::IntegerExpr>(5)
        )
    );

    auto simpleIf = std::make_unique<gash::IfStmt>(
        std::make_unique<gash::BoolExpr>(true),
        std::make_unique<gash::BlockStmt>(
            std::move(simpleStatements)
        ),
        nullptr
    );

    assert(simpleIf->condition != nullptr);
    assert(simpleIf->thenBranch != nullptr);
    assert(simpleIf->elseBranch == nullptr);

    // TEST: Create a while statement
    // while (true) { var x = 10; }

    // Create the loop condition
    auto whileCondition = std::make_unique<gash::BoolExpr>(true);

    // Create the loop body statements
    std::vector<std::unique_ptr<gash::Stmt>> whileStatements;

    whileStatements.push_back(
        std::make_unique<gash::VarDeclStmt>(
            "x",
            std::make_unique<gash::IntegerExpr>(10)
        )
    );

    // Create the loop body block
    auto whileBody = std::make_unique<gash::BlockStmt>(
        std::move(whileStatements)
    );

    // Create the complete while statement
    auto whileLoop = std::make_unique<gash::WhileStmt>(
        std::move(whileCondition),
        std::move(whileBody)
    );

    // Check that the condition and body exist
    assert(whileLoop->condition != nullptr);
    assert(whileLoop->body != nullptr);

    // Check the body contains one statement
    assert(whileLoop->body->statements.size() == 1);

    // Check that the condition is a Boolean expression
    auto* whileBool = dynamic_cast<gash::BoolExpr*>(
        whileLoop->condition.get()
    );

    assert(whileBool != nullptr);
    assert(whileBool->value == true);

    // TEST: Create a loop condition x < 5
    auto comparison = std::make_unique<gash::BinaryExpr>(
        "<",
        std::make_unique<gash::VariableExpr>("x"),
        std::make_unique<gash::IntegerExpr>(5)
    );

    assert(comparison->op == "<");

    // TEST: Create a repeat statement
    // repeat(3) { var x = 10; }

    // Create the repetition count: 3
    auto repeatCount = std::make_unique<gash::IntegerExpr>(3);

    // Create the loop body statements
    std::vector<std::unique_ptr<gash::Stmt>> repeatStatements;

    repeatStatements.push_back(
        std::make_unique<gash::VarDeclStmt>(
            "x",
            std::make_unique<gash::IntegerExpr>(10)
        )
    );

    // Create the loop body block
    auto repeatBody = std::make_unique<gash::BlockStmt>(
        std::move(repeatStatements)
    );

    // Create the repeat statement
    auto repeatLoop = std::make_unique<gash::RepeatStmt>(
        std::move(repeatCount),
        std::move(repeatBody)
    );

    // Check that both parts exist
    assert(repeatLoop->count != nullptr);
    assert(repeatLoop->body != nullptr);

    // Check that the loop body has one statement
    assert(repeatLoop->body->statements.size() == 1);

    // Check that the count is an integer expression
    auto* countExpr = dynamic_cast<gash::IntegerExpr*>(
        repeatLoop->count.get()
    );

    assert(countExpr != nullptr);
    assert(countExpr->value == 3);

    // TEST: Create a repeat loop with count 0
    std::vector<std::unique_ptr<gash::Stmt>> emptyStatements;

    auto zeroRepeat = std::make_unique<gash::RepeatStmt>(
        std::make_unique<gash::IntegerExpr>(0),
        std::make_unique<gash::BlockStmt>(
            std::move(emptyStatements)
        )
    );

    auto* zeroCount = dynamic_cast<gash::IntegerExpr*>(
        zeroRepeat->count.get()
    );

    assert(zeroCount != nullptr);
    assert(zeroCount->value == 0);
    assert(zeroRepeat->body->statements.empty());

    // TEST: Create the statement x = x + 1;

    // Create the assignment target: x
    auto assignmentTarget =
        std::make_unique<gash::VariableExpr>("x");

    // Create the assignment value: x + 1
    auto assignmentValue =
        std::make_unique<gash::BinaryExpr>(
            "+",
            std::make_unique<gash::VariableExpr>("x"),
            std::make_unique<gash::IntegerExpr>(1)
        );

    // Create the assignment expression: x = x + 1
    auto assignment =
        std::make_unique<gash::AssignmentExpr>(
            std::move(assignmentTarget),
            std::move(assignmentValue)
        );

    // Wrap it in a statement: x = x + 1;
    auto expressionStatement =
        std::make_unique<gash::ExpressionStmt>(
            std::move(assignment)
        );

    // Check that the statement contains an expression
    assert(expressionStatement->expression != nullptr);

    // Check that the expression is an assignment
    auto* assignmentNode =
        dynamic_cast<gash::AssignmentExpr*>(
            expressionStatement->expression.get()
        );

    assert(assignmentNode != nullptr);

    // Check the assignment target
    auto* targetNode =
        dynamic_cast<gash::VariableExpr*>(
            assignmentNode->target.get()
        );

    assert(targetNode != nullptr);
    assert(targetNode->name == "x");

    // Check the assignment value is a binary expression
    auto* valueNode =
        dynamic_cast<gash::BinaryExpr*>(
            assignmentNode->value.get()
        );

    assert(valueNode != nullptr);
    assert(valueNode->op == "+");

    // Check the left operand: x
    auto* leftNode = dynamic_cast<gash::VariableExpr*>(
        valueNode->left.get()
    );

    assert(leftNode != nullptr);
    assert(leftNode->name == "x");

    // Check the right operand: 1
    auto* rightNode = dynamic_cast<gash::IntegerExpr*>(
        valueNode->right.get()
    );

    assert(rightNode != nullptr);
    assert(rightNode->value == 1);

    // TEST: Create a function declaration
    // func add(a: int, b: int) { return a + b; }

    // Create the parameters
    std::vector<gash::Parameter> params;

    params.emplace_back("a", "int");
    params.emplace_back("b", "int");

    // Create the return expression: a + b
    auto returnValue = std::make_unique<gash::BinaryExpr>(
        "+",
        std::make_unique<gash::VariableExpr>("a"),
        std::make_unique<gash::VariableExpr>("b")
    );

    // Create the return statement
    auto returnStatement = std::make_unique<gash::ReturnStmt>(
        std::move(returnValue)
    );

    // Add the return statement to the function body
    std::vector<std::unique_ptr<gash::Stmt>> functionStatements;

    functionStatements.push_back(std::move(returnStatement));

    // Create the function body
    auto functionBody = std::make_unique<gash::BlockStmt>(
        std::move(functionStatements)
    );

    // Create the function declaration
    auto function = std::make_unique<gash::FunctionDeclStmt>(
        "add",
        std::move(params),
        std::move(functionBody)
    );

    // Check the function name
    assert(function->name == "add");

    // Check the parameters
    assert(function->parameters.size() == 2);
    assert(function->parameters[0].name == "a");
    assert(function->parameters[0].type == "int");
    assert(function->parameters[1].name == "b");
    assert(function->parameters[1].type == "int");

    // Check the function body
    assert(function->body != nullptr);
    assert(function->body->statements.size() == 1);

    // Check that the first statement is ReturnStmt
    auto* returnNode = dynamic_cast<gash::ReturnStmt*>(
        function->body->statements[0].get()
    );

    assert(returnNode != nullptr);
    assert(returnNode->value != nullptr);

    // Check that the returned expression is a + b
    auto* resultExpr = dynamic_cast<gash::BinaryExpr*>(
        returnNode->value.get()
    );

    assert(resultExpr != nullptr);
    assert(resultExpr->op == "+");

    // Check the left operand: a
    auto* leftParam = dynamic_cast<gash::VariableExpr*>(
        resultExpr->left.get()
    );

    assert(leftParam != nullptr);
    assert(leftParam->name == "a");

    // Check the right operand: b
    auto* rightParam = dynamic_cast<gash::VariableExpr*>(
        resultExpr->right.get()
    );

    assert(rightParam != nullptr);
    assert(rightParam->name == "b");

    // Display success only if all assertions pass
    std::cout << "AST test passed!" << std::endl;

    return 0;
}

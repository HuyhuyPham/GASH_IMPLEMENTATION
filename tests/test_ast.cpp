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

    // TEST: Create list<int>(3)
    auto list = std::make_unique<gash::ListExpr>(
        "int",
        std::make_unique<gash::IntegerExpr>(3)
    );

    // Check the element type
    assert(list->elementType == "int");

    // Check the list size expression
    auto* listSize = dynamic_cast<gash::IntegerExpr*>(
        list->size.get()
    );

    assert(listSize != nullptr);
    assert(listSize->value == 3);

    // TEST: Create numbers[0]
    auto indexExpression = std::make_unique<gash::IndexExpr>(
        std::make_unique<gash::VariableExpr>("numbers"),
        std::make_unique<gash::IntegerExpr>(0)
    );

    // Check the collection
    auto* collectionNode = dynamic_cast<gash::VariableExpr*>(
        indexExpression->collection.get()
    );

    assert(collectionNode != nullptr);
    assert(collectionNode->name == "numbers");

    // Check the index
    auto* indexNode = dynamic_cast<gash::IntegerExpr*>(
        indexExpression->index.get()
    );

    assert(indexNode != nullptr);
    assert(indexNode->value == 0);

    // TEST: Create an exit statement
    auto exitStatement = std::make_unique<gash::ExitStmt>();

    // Check that the node is an ExitStmt
    assert(exitStatement != nullptr);

    // TEST: Create a skip statement
    auto skipStatement = std::make_unique<gash::SkipStmt>();

    // Check that the node is a SkipStmt
    assert(skipStatement != nullptr);

    // TEST: Create a block containing exit and skip statements
    std::vector<std::unique_ptr<gash::Stmt>> loopStatements;

    loopStatements.push_back(
        std::make_unique<gash::ExitStmt>()
    );

    loopStatements.push_back(
        std::make_unique<gash::SkipStmt>()
    );

    auto loopBlock = std::make_unique<gash::BlockStmt>(
        std::move(loopStatements)
    );

    // Check that the block contains two statements
    assert(loopBlock->statements.size() == 2);

    // Check the first statement is ExitStmt
    auto* exitNode = dynamic_cast<gash::ExitStmt*>(
        loopBlock->statements[0].get()
    );

    assert(exitNode != nullptr);

    // Check the second statement is SkipStmt
    auto* skipNode = dynamic_cast<gash::SkipStmt*>(
        loopBlock->statements[1].get()
    );

    assert(skipNode != nullptr);

    // TEST: Create budget(100) { var x = 10; }

    // Create the budget limit: 100
    auto budgetLimit =
        std::make_unique<gash::IntegerExpr>(100);

    // Create the budget body statements
    std::vector<std::unique_ptr<gash::Stmt>> budgetStatements;

    budgetStatements.push_back(
        std::make_unique<gash::VarDeclStmt>(
            "x",
            std::make_unique<gash::IntegerExpr>(10)
        )
    );

    // Create the budget body block
    auto budgetBody = std::make_unique<gash::BlockStmt>(
        std::move(budgetStatements)
    );

    // Create the budget statement
    auto budget = std::make_unique<gash::BudgetStmt>(
        std::move(budgetLimit),
        std::move(budgetBody)
    );

    // Check that both parts exist
    assert(budget->limit != nullptr);
    assert(budget->body != nullptr);

    // Check that the body contains one statement
    assert(budget->body->statements.size() == 1);

    // Check that the limit is an integer expression
    auto* limitExpr = dynamic_cast<gash::IntegerExpr*>(
        budget->limit.get()
    );

    assert(limitExpr != nullptr);
    assert(limitExpr->value == 100);

    // Check that the body contains a variable declaration
    auto* budgetVar = dynamic_cast<gash::VarDeclStmt*>(
        budget->body->statements[0].get()
    );

    assert(budgetVar != nullptr);
    assert(budgetVar->name == "x");

    // TEST: Create guard balance >= 0;

    // Create the guard condition: balance >= 0
    auto guardCondition = std::make_unique<gash::BinaryExpr>(
        ">=",
        std::make_unique<gash::VariableExpr>("balance"),
        std::make_unique<gash::IntegerExpr>(0)
    );

    // Create the guard statement
    auto guardStatement = std::make_unique<gash::GuardStmt>(
        std::move(guardCondition)
    );

    // Check that the guard contains a condition
    assert(guardStatement->condition != nullptr);

    // Check that the condition is a binary expression
    auto* guardBinary = dynamic_cast<gash::BinaryExpr*>(
        guardStatement->condition.get()
    );

    assert(guardBinary != nullptr);
    assert(guardBinary->op == ">=");

    // Check the left operand: balance
    auto* guardVariable = dynamic_cast<gash::VariableExpr*>(
        guardBinary->left.get()
    );

    assert(guardVariable != nullptr);
    assert(guardVariable->name == "balance");

    // Check the right operand: 0
    auto* guardValue = dynamic_cast<gash::IntegerExpr*>(
        guardBinary->right.get()
    );

    assert(guardValue != nullptr);
    assert(guardValue->value == 0);

    // TEST: Create lock balance;
    auto lockStatement =
        std::make_unique<gash::LockStmt>("balance");

    // Check the locked variable name
    assert(lockStatement->name == "balance");

    // TEST: Create unlock balance;
    auto unlockStatement =
        std::make_unique<gash::UnlockStmt>("balance");

    // Check the unlocked variable name
    assert(unlockStatement->name == "balance");

    // TEST: Create a block containing lock and unlock
    std::vector<std::unique_ptr<gash::Stmt>> lockStatements;

    lockStatements.push_back(
        std::make_unique<gash::LockStmt>("balance")
    );

    lockStatements.push_back(
        std::make_unique<gash::UnlockStmt>("balance")
    );

    auto lockBlock = std::make_unique<gash::BlockStmt>(
        std::move(lockStatements)
    );

    // Check the block contains two statements
    assert(lockBlock->statements.size() == 2);

    // Check the first statement is LockStmt
    auto* lockNode = dynamic_cast<gash::LockStmt*>(
        lockBlock->statements[0].get()
    );

    assert(lockNode != nullptr);
    assert(lockNode->name == "balance");

    // Check the second statement is UnlockStmt
    auto* unlockNode = dynamic_cast<gash::UnlockStmt*>(
        lockBlock->statements[1].get()
    );

    assert(unlockNode != nullptr);
    assert(unlockNode->name == "balance");

    // TEST: Create checkpoint save1(x, y);

    // Create the list of variable names
    std::vector<std::string> checkpointVariables;

    checkpointVariables.push_back("x");
    checkpointVariables.push_back("y");

    // Create the checkpoint statement
    auto checkpoint = std::make_unique<gash::CheckpointStmt>(
        "save1",
        std::move(checkpointVariables)
    );

    // Check the checkpoint label
    assert(checkpoint->label == "save1");

    // Check the number of saved variables
    assert(checkpoint->variables.size() == 2);

    // Check the variable names
    assert(checkpoint->variables[0] == "x");
    assert(checkpoint->variables[1] == "y");

    // TEST: Create restore(save1);

    // Create the restore statement
    auto restore = std::make_unique<gash::RestoreStmt>(
        "save1"
    );

    // Check that the checkpoint label is correct
    assert(restore->label == "save1");

    // TEST: Create a block with checkpoint and restore
    std::vector<std::unique_ptr<gash::Stmt>> checkpointStatements;

    checkpointStatements.push_back(
        std::make_unique<gash::CheckpointStmt>(
            "initial",
            std::vector<std::string>{"x", "y"}
        )
    );

    checkpointStatements.push_back(
        std::make_unique<gash::RestoreStmt>("initial")
    );

    auto checkpointBlock = std::make_unique<gash::BlockStmt>(
        std::move(checkpointStatements)
    );

    // Check the block contains two statements
    assert(checkpointBlock->statements.size() == 2);

    // Check the first statement type
    auto* checkpointNode = dynamic_cast<gash::CheckpointStmt*>(
        checkpointBlock->statements[0].get()
    );

    assert(checkpointNode != nullptr);
    assert(checkpointNode->label == "initial");

    // Check the second statement type
    auto* restoreNode = dynamic_cast<gash::RestoreStmt*>(
        checkpointBlock->statements[1].get()
    );

    assert(restoreNode != nullptr);
    assert(restoreNode->label == "initial");

    // TEST: Create inspect(numbers);

    // Create the argument list
    std::vector<std::unique_ptr<gash::Expr>> inspectArgs;

    // Add the variable numbers as an argument
    inspectArgs.push_back(
        std::make_unique<gash::VariableExpr>("numbers")
    );

    // Create the inspect function call
    auto inspectCall = std::make_unique<gash::CallExpr>(
        "inspect",
        std::move(inspectArgs)
    );

    // Wrap the call in an expression statement
    auto inspectStatement =
        std::make_unique<gash::ExpressionStmt>(
            std::move(inspectCall)
        );

    // Check that the statement contains a CallExpr
    auto* inspectNode = dynamic_cast<gash::CallExpr*>(
        inspectStatement->expression.get()
    );

    assert(inspectNode != nullptr);
    assert(inspectNode->callee == "inspect");
    assert(inspectNode->arguments.size() == 1);

    // Check the inspect argument
    auto* inspectVariable = dynamic_cast<gash::VariableExpr*>(
        inspectNode->arguments[0].get()
    );

    assert(inspectVariable != nullptr);
    assert(inspectVariable->name == "numbers");

    // TEST: Create preview();

    // Create an empty argument list
    std::vector<std::unique_ptr<gash::Expr>> previewArgs;

    // Create the preview function call
    auto previewCall = std::make_unique<gash::CallExpr>(
        "preview",
        std::move(previewArgs)
    );

    // Wrap the call in an expression statement
    auto previewStatement =
        std::make_unique<gash::ExpressionStmt>(
            std::move(previewCall)
        );

    // Check the call structure
    auto* previewNode = dynamic_cast<gash::CallExpr*>(
        previewStatement->expression.get()
    );

    assert(previewNode != nullptr);
    assert(previewNode->callee == "preview");
    assert(previewNode->arguments.empty());


    // Display success only if all assertions pass
    std::cout << "AST test passed!" << std::endl;

    return 0;
}


//  Nested statement and boundary tests.

#include <cassert>
#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>

#include "Lexer.h"
#include "Parser.h"

using namespace gash;

// Parse a GASH source string.
Program parseSource(const std::string& source) {
    Lexer lexer(source);
    auto tokens = lexer.tokenize();

    Parser parser(std::move(tokens));
    return parser.parseProgram();
}

int main() {

    // Test 1: Nested if, while and repeat statements.
    {
        Program program = parseSource(
            "if (x > 0) {"
            "  while (x < 10) {"
            "    repeat (2) {"
            "      x = x + 1;"
            "    }"
            "  }"
            "} otherwise {"
            "  output(0);"
            "}"
        );

        assert(program.statements.size() == 1);

        // Check the outer if statement.
        auto* ifStmt = dynamic_cast<IfStmt*>(
            program.statements[0].get()
        );
        assert(ifStmt != nullptr);
        assert(ifStmt->thenBranch != nullptr);
        assert(ifStmt->elseBranch != nullptr);

        // Check the nested while statement.
        auto* whileStmt = dynamic_cast<WhileStmt*>(
            ifStmt->thenBranch->statements[0].get()
        );
        assert(whileStmt != nullptr);

        // Check the nested repeat statement.
        auto* repeatStmt = dynamic_cast<RepeatStmt*>(
            whileStmt->body->statements[0].get()
        );
        assert(repeatStmt != nullptr);

        // Check the assignment inside repeat.
        auto* assignmentStmt = dynamic_cast<ExpressionStmt*>(
            repeatStmt->body->statements[0].get()
        );
        assert(assignmentStmt != nullptr);

        // Check the otherwise block.
        assert(ifStmt->elseBranch->statements.size() == 1);
    }

    // Test 2: Transparency statements inside a budget.
    {
        Program program = parseSource(
            "budget (20) {"
            "  var x = 5;"
            "  checkpoint start(x);"
            "  if (x > 0) {"
            "    guard x > 0;"
            "    lock x;"
            "    unlock x;"
            "  }"
            "  restore(start);"
            "}"
        );

        assert(program.statements.size() == 1);

        auto* budget = dynamic_cast<BudgetStmt*>(
            program.statements[0].get()
        );
        assert(budget != nullptr);
        assert(budget->body->statements.size() == 4);

        // Check the checkpoint.
        auto* checkpoint = dynamic_cast<CheckpointStmt*>(
            budget->body->statements[1].get()
        );
        assert(checkpoint != nullptr);
        assert(checkpoint->label == "start");

        // Check the nested if statement.
        auto* ifStmt = dynamic_cast<IfStmt*>(
            budget->body->statements[2].get()
        );
        assert(ifStmt != nullptr);
        assert(ifStmt->thenBranch->statements.size() == 3);

        // Check guard and lock statements.
        assert(dynamic_cast<GuardStmt*>(
            ifStmt->thenBranch->statements[0].get()
        ) != nullptr);

        assert(dynamic_cast<LockStmt*>(
            ifStmt->thenBranch->statements[1].get()
        ) != nullptr);

        assert(dynamic_cast<UnlockStmt*>(
            ifStmt->thenBranch->statements[2].get()
        ) != nullptr);

        // Check the restore.
        auto* restore = dynamic_cast<RestoreStmt*>(
            budget->body->statements[3].get()
        );
        assert(restore != nullptr);
        assert(restore->label == "start");
    }

    // Test 3: Empty program and nested empty blocks.
    {
        Program empty = parseSource("");
        assert(empty.statements.empty());

        Program nested = parseSource("{{{}}}");
        assert(nested.statements.size() == 1);

        auto* outer = dynamic_cast<BlockStmt*>(
            nested.statements[0].get()
        );
        assert(outer != nullptr);
        assert(outer->statements.size() == 1);

        auto* middle = dynamic_cast<BlockStmt*>(
            outer->statements[0].get()
        );
        assert(middle != nullptr);
        assert(middle->statements.size() == 1);

        auto* inner = dynamic_cast<BlockStmt*>(
            middle->statements[0].get()
        );
        assert(inner != nullptr);
        assert(inner->statements.empty());
    }

    // Test 4: Reject an unclosed nested block.
    {
        bool rejected = false;

        try {
            (void)parseSource(
                "if (true) {"
                "  while (true) {"
                "    repeat (2) { output(1); }"
                "  }"
            );
        } catch (const std::runtime_error&) {
            rejected = true;
        }

        assert(rejected);
    }

    std::cout << "Nested parser tests passed!\n";
    return 0;
}
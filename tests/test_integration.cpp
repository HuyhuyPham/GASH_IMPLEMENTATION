#include "Budget.h"
#include "Checkpoint.h"
#include "CodeGenerator.h"
#include "Environment.h"
#include "Guard.h"
#include "Inspect.h"
#include "Lexer.h"
#include "List.h"
#include "Parser.h"
#include "Semantic.h"
#include "Type.h"
#include "Value.h"

#include <cassert>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

using namespace gash;

int main() {
    std::cout << "============================================================\n";
    std::cout << "          GASH FULL SYSTEM INTEGRATION TEST SUITE           \n";
    std::cout << "============================================================\n";

    // Integration Test 1: Full Frontend-to-CodeGen Pipeline (Lexer -> Parser -> Semantic -> CodeGenerator)
    {
        const std::string source = R"(
            func square(int n) {
                return n * n;
            }

            func fact(int n) {
                if (n > 1) {
                    return n * fact(n - 1);
                }
                return 1;
            }

            var x = 5;
            var y = square(x) + int("10") + fact(3);
            var nums = list<int>(3);
            nums[0] = y;
            guard x >= 0;
            checkpoint cp1(x, nums);
            lock y;
            unlock y;
            restore(cp1);

            budget (50) {
                while (x > 0) {
                    x = x - 1;
                    if (x == 2) {
                        skip;
                    }
                    if (x == 1) {
                        exit;
                    }
                }
            }

            inspect(nums);
            preview();
            output(nums[0]);
        )";

        Lexer lexer(source);
        auto tokens = lexer.tokenize();
        Parser parser(std::move(tokens));
        Program program = parser.parseProgram();

        SemanticAnalyzer semantic;
        semantic.analyze(program);
        assert(semantic.getFunctionReturnType("square") == Type::Int());
        assert(semantic.getFunctionReturnType("fact") == Type::Int());

        CodeGenerator codegen;
        std::string cppCode = codegen.generate(program);
        assert(cppCode.find("int gash_square(int gash_n)") != std::string::npos);
        assert(cppCode.find("int gash_fact(int gash_n)") != std::string::npos);
        assert(cppCode.find("gash::toInt") != std::string::npos);
        assert(cppCode.find("gash::List<int>") != std::string::npos);
        assert(cppCode.find(".set(0,") != std::string::npos);
        assert(cppCode.find("gash::BudgetScope") != std::string::npos);
        assert(cppCode.find("gash::inspect") != std::string::npos);
        assert(cppCode.find("gash::preview") != std::string::npos);
        assert(cppCode.find("gash_env.addLiveGuard") != std::string::npos);
        assert(cppCode.find("gash_env.lock(\"y\")") != std::string::npos);
        assert(cppCode.find("gash_env.unlock(\"y\")") != std::string::npos);
        assert(cppCode.find("gash_checkpoints.save(\"cp1\"") != std::string::npos);
        assert(cppCode.find("gash_checkpoints.restore(\"cp1\"") != std::string::npos);

        std::cout << "[PASS] Integration 1: Lexer -> Parser -> Semantic -> CodeGenerator pipeline\n";
    }

    // Integration Test 2: Runtime Integration (Environment + Guards + Locks + Checkpoints + List + Inspect + Preview + Budget)
    {
        Environment env;
        CheckpointManager checkpoints;

        int liveBalance = 500;
        env.bindReference("balance", liveBalance);

        Lexer guardLex("balance >= 0 && balance <= 1000");
        Parser guardParser(guardLex.tokenize());
        auto guardExpr = guardParser.parseExpressionOnly();
        env.addExpressionGuard(*guardExpr);

        checkpoints.save("initial", {"balance"}, env);

        {
            BudgetScope budget(10);
            BudgetScope::check();
            BudgetScope::tick();
            env.assign("balance", Value::Int(toInt("750")));
            assert(liveBalance == 750);
        }

        // Test List<int> + inspect() + Environment preview
        List<int> runtimeList(2);
        runtimeList.set(0, liveBalance);
        std::ostringstream inspectOut;
        inspect(runtimeList, inspectOut);
        assert(inspectOut.str().find("[0] 750") != std::string::npos);
        assert(inspectOut.str().find("[1] <uninitialized>") != std::string::npos);

        registerPreviewEnvironment(&env);
        std::ostringstream previewOut;
        previewActiveVariables(previewOut);
        assert(previewOut.str().find("balance : int = 750 [unlocked] scope=global") != std::string::npos);
        clearPreviewEnvironment();

        // Restore checkpoint and verify live C++ variable rolled back
        checkpoints.restore("initial", env);
        assert(liveBalance == 500);

        std::cout << "[PASS] Integration 2: Environment + Guards + Locks + Checkpoint + List + Inspect + Preview + Budget\n";
    }

    // Integration Test 3: Generated C++ Mutation Wrappers (mutateVar, mutateList, mutateListVoid, addLiveGuard)
    {
        Environment env;
        CheckpointManager checkpoints;

        int counter = 10;
        List<int> items(2);
        items.set(0, 100);

        env.bindReference("counter", counter);
        env.bindReference("items", items);
        env.addLiveGuard([&]() -> bool { return counter >= 0 && items.get(0) >= 0; });

        checkpoints.save("snap", {"counter", "items"}, env);

        env.mutateVar("counter", [&]() { return (counter = 25); });
        assert(counter == 25);

        env.mutateList("items", [&]() { return (items.set(1, 200), 200); });
        assert(items.get(1) == 200);

        env.mutateListVoid("items", [&]() { items.resize(4); });
        assert(items.size() == 4);

        // Guard violation on mutateVar must roll back counter to 25
        bool caughtGuard = false;
        try {
            env.mutateVar("counter", [&]() { return (counter = -1); });
        } catch (const std::runtime_error&) {
            caughtGuard = true;
        }
        assert(caughtGuard);
        assert(counter == 25);

        // Lock violation on mutateList must reject write before modifying items
        env.lock("items");
        bool caughtLock = false;
        try {
            env.mutateList("items", [&]() { return (items.set(0, 999), 999); });
        } catch (const std::runtime_error&) {
            caughtLock = true;
        }
        assert(caughtLock);
        assert(items.get(0) == 100);
        env.unlock("items");

        // Restore checkpoint and verify both counter and items rolled back
        checkpoints.restore("snap", env);
        assert(counter == 10);
        assert(items.size() == 2);
        assert(items.get(0) == 100);
        assert(!items.isInitialized(1));

        std::cout << "[PASS] Integration 3: Generated C++ mutation wrappers & list checkpoint restore\n";
    }

    std::cout << "============================================================\n";
    std::cout << "KET QUA: 3/3 bai test tich hop toan he thong da PASS!\n";
    std::cout << "============================================================\n";
    return 0;
}

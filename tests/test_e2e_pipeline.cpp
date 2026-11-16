// End-to-end compiler pipeline tests (Lex -> Parse -> Semantic -> Codegen -> g++ -> Run).
#if __has_include("../include/CompilerPipeline.h")
#include "../include/CompilerPipeline.h"
#include "../include/Lexer.h"
#include "../include/LexicalError.h"
#include "../include/Parser.h"
#include "../include/Semantic.h"
#else
#include "CompilerPipeline.h"
#include "Lexer.h"
#include "LexicalError.h"
#include "Parser.h"
#include "Semantic.h"
#endif

#include <cassert>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

namespace fs = std::filesystem;

namespace {

struct RunResult {
    int exitCode = 0;
    std::string stdoutText;
    std::string stderrText;
};

// Normalizes CRLF line endings to LF for cross-platform output comparison.
std::string normalizeNewlines(std::string text) {
    std::string out;
    out.reserve(text.size());
    for (char c : text) {
        if (c != '\r') {
            out.push_back(c);
        }
    }
    return out;
}

// Reads a text output file captured from an executed test binary.
std::string readTextFile(const fs::path& p) {
    std::ifstream in(p, std::ios::binary);
    if (!in) {
        return "";
    }
    std::ostringstream oss;
    oss << in.rdbuf();
    return normalizeNewlines(oss.str());
}

// Compiles GASH source through CompilerPipeline and executes the resulting binary.
RunResult compileAndExecuteGash(const std::string& testName, const std::string& source) {
    gash::Lexer lexer(source);
    auto tokens = lexer.tokenize();
    gash::Parser parser(std::move(tokens));
    gash::Program program = parser.parseProgram();
    gash::SemanticAnalyzer semantic;
    semantic.analyze(program);

    fs::path tempDir = fs::temp_directory_path() / "gash_e2e_tests" / testName;
    fs::create_directories(tempDir);
    fs::path cppPath = tempDir / "program.cpp";
#ifdef _WIN32
    fs::path exePath = tempDir / "program.exe";
#else
    fs::path exePath = tempDir / "program.out";
#endif
    fs::path stdoutPath = tempDir / "stdout.txt";
    fs::path stderrPath = tempDir / "stderr.txt";

    gash::CompilerPipeline pipeline;
    pipeline.writeGeneratedCpp(program, cppPath);
    pipeline.compileGeneratedCpp(cppPath, exePath);

    std::string cmd = "\"" + fs::absolute(exePath).string() + "\" > \"" +
                      fs::absolute(stdoutPath).string() + "\" 2> \"" +
                      fs::absolute(stderrPath).string() + "\"";
#ifdef _WIN32
    cmd = "\"" + cmd + "\"";
#endif
    int rawStatus = std::system(cmd.c_str());

    RunResult result;
#ifdef _WIN32
    result.exitCode = rawStatus;
#else
    result.exitCode = (rawStatus == -1) ? -1 : ((rawStatus >> 8) & 0xFF);
#endif
    result.stdoutText = readTextFile(stdoutPath);
    result.stderrText = readTextFile(stderrPath);

    std::error_code ec;
    fs::remove_all(tempDir, ec);
    return result;
}

// Verifies a GASH program fails at runtime with non-zero exit code and expected diagnostic.
void expectRuntimeFailure(
    const std::string& testName,
    const std::string& source,
    const std::string& expectedErrorFragment
) {
    RunResult res = compileAndExecuteGash(testName, source);
    if (res.exitCode == 0) {
        throw std::runtime_error(testName + ": expected non-zero exit code");
    }
    if (res.stderrText.find(expectedErrorFragment) == std::string::npos) {
        throw std::runtime_error(
            testName + ": expected stderr to contain '" + expectedErrorFragment +
            "', got: " + res.stderrText);
    }
}

// Verifies a GASH snippet fails during compilation with the expected exception type.
template <typename ExceptionT>
void expectCompileStageError(
    const std::string& testName,
    const std::string& source,
    const std::string& expectedFragment
) {
    try {
        gash::Lexer lexer(source);
        auto tokens = lexer.tokenize();
        gash::Parser parser(std::move(tokens));
        gash::Program program = parser.parseProgram();
        gash::SemanticAnalyzer semantic;
        semantic.analyze(program);
    } catch (const ExceptionT& err) {
        if (std::string(err.what()).find(expectedFragment) == std::string::npos) {
            throw std::runtime_error(
                testName + ": error message '" + err.what() +
                "' did not contain '" + expectedFragment + "'");
        }
        return;
    }
    throw std::runtime_error(testName + ": expected compilation error but succeeded");
}

} // namespace

// Runs all 6 end-to-end GASH compilation, execution, and error-diagnostic test groups.
int main() {
    std::cout << "Running comprehensive GASH end-to-end compiler pipeline tests...\n";

    // 1. Expressions, precedence (^ right-associative, unary -), booleans, chars, conversions
    {
        RunResult res = compileAndExecuteGash("expressions_and_types", R"(
            var a = -2 ^ 2;
            var b = 2 ^ 3 ^ 2;
            var c = '\'' ;
            var nul = '\0';
            var flag = true;
            output(a);
            output(b);
            output(c);
            output(flag);
            output(!flag);
            output(string(15) + "_ok");
            output(int("42") + 8);
            output(float("2.5") * 2.0);
        )");
        assert(res.exitCode == 0);
        assert(res.stdoutText == "-4\n512\n'\ntrue\nfalse\n15_ok\n50\n5\n");
        std::cout << "[PASS] 1. Precedence, escapes, booleans, and explicit conversions\n";
    }

    // 2. Global variables inside functions, recursion, list parameters, nested local functions
    {
        RunResult res = compileAndExecuteGash("globals_lists_nested_funcs", R"(
            var total = 10;

            func addToTotal(int delta) {
                total = total + delta;
                return total;
            }

            func firstPlusLast(list<int> items, int len) {
                return items[0] + items[len - 1];
            }

            func factorial(int n) {
                if (n <= 1) {
                    return 1;
                }
                return n * factorial(n - 1);
            }

            output(addToTotal(5));
            output(total);

            var nums = list<int>(3);
            nums[0] = 4;
            nums[2] = 9;
            output(firstPlusLast(nums, 3));
            output(factorial(5));

            {
                func doubleIt(int v) {
                    return v * 2;
                }
                output(doubleIt(7));
            }
        )");
        assert(res.exitCode == 0);
        assert(res.stdoutText == "15\n15\n13\n120\n14\n");
        std::cout << "[PASS] 2. Globals in functions, list<T> params, recursion, nested functions\n";
    }

    // 3. Control flow, lexical shadowing, list value-copy semantics, inspect(), extend()
    {
        RunResult res = compileAndExecuteGash("control_flow_and_lists", R"(
            var x = 100;
            {
                var x = 5;
                output(x);
            }
            output(x);

            var a = list<string>(2);
            a[0] = "alpha";
            var b = a;
            b[0] = "beta";
            output(a[0]);
            output(b[0]);

            repeat (0) {
                output("never");
            }
            repeat (2) {
                output("step");
            }
            extend(a, 3);
            inspect(a);
        )");
        assert(res.exitCode == 0);
        assert(res.stdoutText.find("5\n100\nalpha\nbeta\nstep\nstep\nList<string> size=3 capacity=") == 0);
        assert(res.stdoutText.find("  [0] \"alpha\"\n  [1] <uninitialized>\n  [2] <uninitialized>\n") != std::string::npos);
        std::cout << "[PASS] 3. Shadowing, list value-copy semantics, repeat, extend, inspect\n";
    }

    // 4. Guards, locks, preview, repeatable checkpoints, and nested budgets (outermost precedence)
    {
        RunResult res = compileAndExecuteGash("transparency_e2e", R"(
            var balance = 100;
            guard balance >= 0;
            checkpoint init(balance);

            balance = 40;
            lock balance;
            preview();
            unlock balance;

            restore(init);
            output(balance);

            var count = 0;
            budget (20) {
                budget (3) {
                    while (true) {
                        count = count + 1;
                    }
                }
                output("after inner budget");
            }
            output(count);
        )");
        assert(res.exitCode == 0);
        assert(res.stdoutText.find("balance : int = 40 [locked] scope=global") != std::string::npos);
        assert(res.stdoutText.find("100\nafter inner budget\n") != std::string::npos);
        std::cout << "[PASS] 4. Guard, lock/unlock, preview, checkpoint/restore, nested budgets\n";
    }

    // 5. Compilation error diagnostics (Lexical, Syntax with location, Semantic with location)
    {
        expectCompileStageError<gash::LexicalError>("lexical_error", "var x = @;", "Unexpected character");
        expectCompileStageError<std::runtime_error>("syntax_error", "var x = 10", "Syntax error at line");
        expectCompileStageError<gash::SemanticError>("semantic_error", "var x = 10 + 2.5;", "Semantic error at line");
        std::cout << "[PASS] 5. Lexical, syntax, and semantic diagnostic errors with locations\n";
    }

    // 6. Runtime error diagnostics (clean non-zero exit + Runtime error message)
    {
        expectRuntimeFailure("div_zero", "var a = 10; var b = 0; output(a / b);", "Division by zero");
        expectRuntimeFailure("mod_zero", "var a = 10; var b = 0; output(a % b);", "Modulo by zero");
        expectRuntimeFailure("neg_repeat", "var n = -1; repeat (n) { output(1); }", "Repeat count cannot be negative");
        expectRuntimeFailure("bad_int_cast", "var x = int(\"abc\");", "Cannot convert string");
        expectRuntimeFailure("uninit_list_read", "var a = list<int>(2); output(a[0]);", "uninitialized list element");
        expectRuntimeFailure("oob_list_read", "var a = list<int>(2); output(a[5]);", "out of bounds");
        expectRuntimeFailure("guard_fail", "var x = 10; guard x >= 0; x = -5;", "Guard violation");
        expectRuntimeFailure("lock_fail", "var x = 10; lock x; x = 20;", "Cannot modify locked variable");
        std::cout << "[PASS] 6. All runtime error categories fail cleanly without aborting\n";
    }

    std::cout << "ALL E2E COMPILER PIPELINE TESTS PASSED!\n";
    return 0;
}

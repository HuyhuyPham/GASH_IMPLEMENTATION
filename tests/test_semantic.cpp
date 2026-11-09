#include "Lexer.h"
#include "Parser.h"
#include "Semantic.h"
#include "Type.h"

#include <cassert>
#include <functional>
#include <iostream>
#include <stdexcept>
#include <string>

using namespace gash;

namespace {

Program parseSource(const std::string& source) {
    Lexer lexer(source);
    auto tokens = lexer.tokenize();
    Parser parser(std::move(tokens));
    return parser.parseProgram();
}

void analyzeValid(const std::string& source) {
    Program prog = parseSource(source);
    SemanticAnalyzer analyzer;
    analyzer.analyze(prog);
}

bool failsSemantic(const std::string& source) {
    Program prog = parseSource(source);
    SemanticAnalyzer analyzer;
    try {
        analyzer.analyze(prog);
    } catch (const SemanticError&) {
        return true;
    }
    return false;
}

void runTest(
    int id,
    const std::string& title,
    const std::string& whatTested,
    const std::string& howTested,
    const std::function<void()>& testFn,
    int& passedCount
) {
    std::cout << "------------------------------------------------------------\n";
    std::cout << "[TEST " << id << "] " << title << "\n";
    std::cout << "  - Noi dung (What): " << whatTested << "\n";
    std::cout << "  - Cach test (How): " << howTested << "\n";
    testFn();
    ++passedCount;
    std::cout << "  - Trang thai     : [PASS]\n";
}

} // namespace

int main() {
    int passed = 0;
    const int total = 9;

    std::cout << "============================================================\n";
    std::cout << "       GASH SEMANTIC ANALYZER & TYPE SYSTEM TEST SUITE      \n";
    std::cout << "============================================================\n";

    // Test 1: Type model, C++ mapping (A27), and ScopeStack basics
    runTest(
        1,
        "Type Model, C++ Mapping (A27) & Lexical ScopeStack",
        "Kiem tra he thong kieu (int, float, bool, char, string, void, list<T>), "
        "anh xa sang kieu C++17 (A27), va ngan xep pham vi ScopeStack.",
        "Goi Type::fromName(), toString(), toCppType() va thu khai bao/tim kiem "
        "Symbol qua cac tang pushScope()/popScope().",
        [] {
            assert(Type::Int() == Type::fromName("int"));
            assert(Type::Float() == Type::fromName("float"));
            assert(Type::Bool() == Type::fromName("bool"));
            assert(Type::Char() == Type::fromName("char"));
            assert(Type::String() == Type::fromName("string"));
            assert(Type::Void() == Type::fromName("void"));

            Type intList = Type::fromName("list<int>");
            assert(intList.isList());
            assert(intList.elementType() == Type::Int());
            assert(intList.toString() == "list<int>");
            assert(intList != Type::fromName("list<float>"));
            assert(!Type::fromName("list<list<int>>").isValid());

            // Assumption A27: C++ type mapping
            assert(Type::Int().toCppType() == "int");
            assert(Type::Float().toCppType() == "double");
            assert(Type::String().toCppType() == "std::string");
            assert(Type::Bool().toCppType() == "bool");
            assert(Type::Char().toCppType() == "char");
            assert(Type::Void().toCppType() == "void");
            assert(intList.toCppType() == "gash::List<int>");

            ScopeStack scopes;
            assert(scopes.depth() == 1);
            assert(scopes.declare(Symbol{"x", SymbolKind::Variable, Type::Int()}));
            assert(!scopes.declare(Symbol{"x", SymbolKind::Variable, Type::Float()}));

            scopes.pushScope();
            assert(scopes.depth() == 2);
            assert(scopes.declare(Symbol{"x", SymbolKind::Variable, Type::String()}));
            assert(scopes.lookup("x")->type == Type::String());
            scopes.popScope();
            assert(scopes.lookup("x")->type == Type::Int());
        },
        passed);

    // Test 2: Valid variable declarations, strict expressions, and casts (A31, A40)
    runTest(
        2,
        "Valid Declarations, Expressions & Explicit Type Casts (A31, A40)",
        "Kiem tra suy dien kieu bien (var), bieu thuc so hoc/logic hop le, "
        "va ep kieu tuong minh int(), float(), string().",
        "Parse doan code GASH chua cac phep toan +, -, *, %, ^, &&, ||, ! "
        "va cac ham ep kieu, sau do chay SemanticAnalyzer::analyze().",
        [] {
            analyzeValid(R"(
                var a = 10;
                var b = 20;
                var sum = a + b * 2 - (a % 3) ^ 2;
                var f1 = 3.5;
                var f2 = float(a) + f1;
                var i1 = int(f1);
                var s1 = "Hello, " + string(a);
                var c1 = 'A';
                var ok = (a < b) && (f1 >= 1.0) || !(c1 == 'B');
            )");
        },
        passed);

    // Test 3: Strict typing rejects mixed int/float arithmetic and invalid operators
    runTest(
        3,
        "Strict Typing Enforcement & Type Error Rejection",
        "Dam bao he thong tu choi moi phep toan khac kieu (nhu int + float, "
        "string + int, float % float, !int, int && bool).",
        "Dua 9 bieu thuc sai kieu vao SemanticAnalyzer va xac nhan tat ca "
        "deu nem ngoai le SemanticError.",
        [] {
            assert(failsSemantic("var x = 10 + 3.5;"));
            assert(failsSemantic("var x = 10 < 3.5;"));
            assert(failsSemantic("var x = 10 == 3.5;"));
            assert(failsSemantic("var x = 3.5 % 1.5;"));
            assert(failsSemantic("var x = \"hello\" + 5;"));
            assert(failsSemantic("var x = \"hello\" - \"world\";"));
            assert(failsSemantic("var x = !10;"));
            assert(failsSemantic("var x = -\"abc\";"));
            assert(failsSemantic("var x = 1 && true;"));
        },
        passed);

    // Test 4: Variable redeclaration, undeclared variables, and scope shadowing
    runTest(
        4,
        "Variable Scope Resolution, Redeclaration & Shadowing",
        "Kiem tra loi bien chua khai bao, khai bao trung trong cung scope, "
        "gan sai kieu, che khuat bien (shadowing) o block con va gioi han pham vi block.",
        "Chay analyze() tren cac mau code khai bao trung/chua khai bao (phai bao loi) "
        "va mau code shadowing hop le trong block lồng nhau.",
        [] {
            assert(failsSemantic("var x = y + 1;"));
            assert(failsSemantic("var x = 1; var x = 2;"));
            assert(failsSemantic("var x = 1; x = \"wrong\";"));

            analyzeValid(R"(
                var x = 10;
                {
                    var x = "shadowed";
                    var y = x + "!";
                }
                x = x + 5;
            )");

            assert(failsSemantic(R"(
                {
                    var inner = 42;
                }
                var outer = inner;
            )"));
        },
        passed);

    // Test 5: Functions with return type inference, recursion (A29), and void functions
    runTest(
        5,
        "Function Signatures, Return Type Inference, Recursion (A29) & Void Functions",
        "Kiem tra suy dien kieu tra ve cua ham co return, ham de quy (factorial), "
        "va tu dong suy dien kieu void khi ham khong co lenh return.",
        "Phan tich ham add(int, int), logValue(int) va ham de quy factorial(int), "
        "kiem tra Symbol::type va getFunctionReturnType().",
        [] {
            Program prog = parseSource(R"(
                func add(int a, int b) {
                    return a + b;
                }

                func logValue(int v) {
                    output(v);
                }

                var res = add(3, 4);
                logValue(res);
            )");

            SemanticAnalyzer analyzer;
            analyzer.analyze(prog);

            const Symbol* addSym = analyzer.lookup("add");
            assert(addSym != nullptr);
            assert(addSym->kind == SymbolKind::Function);
            assert(addSym->type == Type::Int());
            assert(addSym->hasReturnValue);

            const Symbol* logSym = analyzer.lookup("logValue");
            assert(logSym != nullptr);
            assert(logSym->kind == SymbolKind::Function);
            assert(logSym->type == Type::Void());
            assert(!logSym->hasReturnValue);

            // Assumption A29: Recursive function return type inference
            Program recProg = parseSource(R"(
                func factorial(int n) {
                    if (n > 1) {
                        return n * factorial(n - 1);
                    }
                    return 1;
                }
                var f5 = factorial(5);
            )");
            SemanticAnalyzer recAnalyzer;
            recAnalyzer.analyze(recProg);
            assert(recAnalyzer.getFunctionReturnType("factorial") == Type::Int());
            assert(recAnalyzer.lookup("f5")->type == Type::Int());
        },
        passed);

    // Test 6: Function error cases (inconsistent returns, bad args, void assignment)
    runTest(
        6,
        "Function Semantic Error Cases",
        "Dam bao bat loi khi ham co nhieu lenh return khac kieu, gan ket qua "
        "ham void cho bien, sai so luong/kieu doi so, trung ten tham so, hoac return ngoai ham.",
        "Kiem tra 6 truong hop loi ham khac nhau bang failsSemantic().",
        [] {
            // Inconsistent return types in the same function
            assert(failsSemantic(R"(
                func bad(int x) {
                    if (x > 0) {
                        return 1;
                    } otherwise {
                        return "one";
                    }
                }
            )"));

            // Assign result of void function to variable
            assert(failsSemantic(R"(
                func doNothing(int x) {
                    output(x);
                }
                var a = doNothing(5);
            )"));

            // Wrong argument count or type
            assert(failsSemantic(R"(
                func add(int a, int b) { return a + b; }
                var x = add(1);
            )"));
            assert(failsSemantic(R"(
                func add(int a, int b) { return a + b; }
                var x = add(1, 2.5);
            )"));

            // Duplicate parameter names
            assert(failsSemantic(R"(
                func dup(int a, float a) { return a; }
            )"));

            // Return outside of function
            assert(failsSemantic("return 10;"));
        },
        passed);

    // Test 7: Control flow conditions and loop context checks (exit/skip - A32)
    runTest(
        7,
        "Control Flow Conditions & Loop Context Validation (A32)",
        "Kiem tra dieu kien if/while phai la bool, repeat count phai la int, "
        "va exit/skip chi duoc phep xuat hien ben trong vong lap (A32).",
        "Kiem tra vong lap while/repeat hop le va cac truong hop dung exit/skip "
        "ngoai vong lap hoac trong ham khai bao ben trong vong lap.",
        [] {
            analyzeValid(R"(
                var i = 0;
                while (i < 10) {
                    i = i + 1;
                    if (i == 3) {
                        skip;
                    }
                    if (i == 8) {
                        exit;
                    }
                }
                repeat (5) {
                    output(i);
                    exit;
                }
            )");

            assert(failsSemantic("if (123) { output(1); }"));
            assert(failsSemantic("while (\"true\") { output(1); }"));
            assert(failsSemantic("repeat (3.14) { output(1); }"));
            assert(failsSemantic("exit;"));
            assert(failsSemantic("skip;"));

            // exit inside a function that is declared inside a loop is invalid
            assert(failsSemantic(R"(
                while (true) {
                    func helper(int x) {
                        exit;
                        return x;
                    }
                    exit;
                }
            )"));
        },
        passed);

    // Test 8: Lists, indexing, built-ins, and casts validation (A31)
    runTest(
        8,
        "List Types, Indexing & Built-in Functions Validation (A31)",
        "Kiem tra khoi tao list<T>(size), truy xuat phan tu list[i], "
        "va cac ham tich hop input(), output(), inspect(), preview(), extend().",
        "Phan tich cac thao tac list hop le va kiem tra bat loi khi size am, "
        "index khong phai int, gan phan tu sai kieu, hoac goi inspect/extend sai kieu.",
        [] {
            analyzeValid(R"(
                var nums = list<int>(4);
                nums[0] = 10;
                var first = nums[0];
                extend(nums, 8);
                inspect(nums);
                preview();
                var line = input();
            )");

            assert(failsSemantic("var badList = list<int>(true);"));
            assert(failsSemantic("var badList = list<int>(-3);"));
            assert(failsSemantic("var nums = list<int>(3); var x = nums[\"0\"];"));
            assert(failsSemantic("var nums = list<int>(3); nums[0] = 3.14;"));
            assert(failsSemantic("var x = 10; var y = x[0];"));
            assert(failsSemantic("var x = 10; inspect(x);"));
            assert(failsSemantic("var nums = list<int>(3); extend(nums, \"10\");"));
            assert(failsSemantic("var nums = list<int>(3); var c = int(nums);"));
        },
        passed);

    // Test 9: Transparency statements (guard, lock, unlock, checkpoint, restore, budget - A34)
    runTest(
        9,
        "Transparency Statements Semantic Validation (A34)",
        "Kiem tra tinh hop le tinh cua guard (bieu thuc bool), lock/unlock (bien ton tai), "
        "checkpoint/restore (nhan duy nhat, khong trung bien, cung scope/kieu) va budget (int).",
        "Kiem tra kich ban su dung transparency hop le va 8 kich ban vi pham quy tac.",
        [] {
            analyzeValid(R"(
                var balance = 100;
                guard balance >= 0;
                checkpoint cp1(balance);
                lock balance;
                unlock balance;
                restore(cp1);
                budget (50) {
                    balance = balance + 10;
                }
            )");

            assert(failsSemantic("var x = 10; guard x + 5;"));
            assert(failsSemantic("guard missing >= 0;"));
            assert(failsSemantic("lock missing;"));
            assert(failsSemantic("unlock missing;"));
            assert(failsSemantic("checkpoint cp1(missing);"));
            assert(failsSemantic("var x = 1; checkpoint cp1(x, x);"));
            assert(failsSemantic("var x = 1; checkpoint cp1(x); checkpoint cp1(x);"));
            assert(failsSemantic("restore(unknownCp);"));
            assert(failsSemantic("budget (true) { output(1); }"));
        },
        passed);

    std::cout << "============================================================\n";
    std::cout << "KET QUA: " << passed << "/" << total
              << " bai test Semantic Analysis da PASS thanh cong!\n";
    std::cout << "============================================================\n";
    return 0;
}

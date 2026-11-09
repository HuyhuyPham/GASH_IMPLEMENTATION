#include "Checkpoint.h"
#include "Environment.h"
#include "Guard.h"
#include "Lexer.h"
#include "Parser.h"
#include "Value.h"

#include <cassert>
#include <functional>
#include <iostream>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>

using namespace gash;

namespace {

template <typename Fn>
bool throwsRuntimeError(Fn&& fn) {
    try {
        fn();
    } catch (const std::runtime_error&) {
        return true;
    }
    return false;
}

std::unique_ptr<Expr> parseExpr(const std::string& exprSource) {
    Lexer lexer(exprSource);
    auto tokens = lexer.tokenize();
    Parser parser(std::move(tokens));
    return parser.parseExpressionOnly();
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
    std::cout << "   GASH ENVIRONMENT, LOCKS, GUARDS & CHECKPOINT TEST SUITE  \n";
    std::cout << "============================================================\n";

    // Test 1: Environment variable state, scope frames, shadowing, and central assignment API (A30)
    runTest(
        1,
        "Environment State, Scope Frames & Central Assignment API (A30)",
        "Kiem tra luu tru bien, quan ly scope frames (pushScope/popScope), "
        "shadowing bien o scope con, va kiem tra kieu trong Environment::assign().",
        "Khai bao bien o global scope va inner scope, goi assign() voi kieu dung "
        "va kieu sai (phai nem runtime_error va giu nguyen gia tri cu).",
        [] {
            Environment env;
            assert(env.scopeDepth() == 1);
            assert(env.currentScopeId() == 1);

            env.define("x", Value::Int(10));
            env.define("name", Value::String("Alice"));
            assert(env.get("x").asInt() == 10);
            assert(env.get("name").asString() == "Alice");

            // Central assignment enforces strict types
            env.assign("x", Value::Int(25));
            assert(env.get("x").asInt() == 25);
            assert(throwsRuntimeError([&] { env.assign("x", Value::Float(25.0)); }));
            assert(env.get("x").asInt() == 25);

            // Nested scope shadowing
            std::size_t innerId = env.pushScope();
            assert(innerId == 2);
            env.define("x", Value::String("inner_x"));
            assert(env.get("x").asString() == "inner_x");
            env.assign("x", Value::String("updated_inner"));
            assert(env.get("x").asString() == "updated_inner");

            env.popScope();
            assert(env.get("x").asInt() == 25);
        },
        passed);

    // Test 2: Lock and unlock enforcement, including no-op repeat calls (A33)
    runTest(
        2,
        "Lock & Unlock Permissions and No-Op Repeats (A33)",
        "Kiem tra co che khoa/mo khoa bien, chan ghi khi dang lock, "
        "va dam bao goi lock/unlock lap lai nhieu lan la thao tac an toan (no-op).",
        "Goi unlock() khi chua khoa, goi lock() 2 lan lien tiep, thu assign() (phai bi chan), "
        "sau do unlock() 2 lan va assign() lai.",
        [] {
            Environment env;
            env.define("balance", Value::Int(100));
            assert(!env.isLocked("balance"));

            // Unlocking an already-unlocked variable is a no-op
            env.unlock("balance");
            assert(!env.isLocked("balance"));

            // Lock variable and verify repeat lock is a no-op
            env.lock("balance");
            assert(env.isLocked("balance"));
            env.lock("balance");
            assert(env.isLocked("balance"));

            // Mutation is blocked while locked
            assert(throwsRuntimeError([&] { env.assign("balance", Value::Int(200)); }));
            assert(env.get("balance").asInt() == 100);

            // Unlock and verify repeat unlock is a no-op
            env.unlock("balance");
            assert(!env.isLocked("balance"));
            env.unlock("balance");
            assert(!env.isLocked("balance"));

            // Mutation succeeds after unlock
            env.assign("balance", Value::Int(200));
            assert(env.get("balance").asInt() == 200);

            // Locking/unlocking undeclared variable fails
            assert(throwsRuntimeError([&] { env.lock("missing"); }));
            assert(throwsRuntimeError([&] { env.unlock("missing"); }));
        },
        passed);

    // Test 3: List locking blocks element writes, whole-list assignment, and resizing (A13, A14, A46)
    runTest(
        3,
        "List Mutation API & List Locking Enforcement (A13, A14, A46)",
        "Kiem tra ListValue::set() danh dau khoi tao, kiem tra can, va khi list bi lock "
        "thi chan sua phan tu, chan extendList va chan gan lai toan bo list.",
        "Tao list<int>(3), gan phan tu, lock list, thu doc (cho phep) va thu "
        "assignListElement / extendList / assign (deu phai bi chan), sau do unlock.",
        [] {
            Environment env;
            env.define("nums", Value::List(Type::Int(), 3));
            env.assignListElement("nums", 0, Value::Int(10));
            env.assignListElement("nums", 1, Value::Int(20));
            assert(env.getListElement("nums", 0).asInt() == 10);

            env.lock("nums");
            assert(env.isLocked("nums"));

            // Reading from a locked list is still permitted
            assert(env.getListElement("nums", 0).asInt() == 10);
            assert(env.get("nums").asList().size() == 3);

            // Element mutation, extend/resize, and whole-list replacement are blocked
            assert(throwsRuntimeError([&] {
                env.assignListElement("nums", 0, Value::Int(99));
            }));
            assert(throwsRuntimeError([&] {
                env.extendList("nums", 5);
            }));
            assert(throwsRuntimeError([&] {
                env.assign("nums", Value::List(Type::Int(), 2));
            }));
            assert(env.getListElement("nums", 0).asInt() == 10);
            assert(env.get("nums").asList().size() == 3);

            // Unlocking restores list mutation permissions
            env.unlock("nums");
            env.assignListElement("nums", 2, Value::Int(30));
            env.extendList("nums", 5);
            assert(env.getListElement("nums", 2).asInt() == 30);
            assert(env.get("nums").asList().size() == 5);
        },
        passed);

    // Test 4: Guard type validation, initial value validation, and candidate assignment checks (A33)
    runTest(
        4,
        "Mutation Guards: Type Validation, Initial Value & Candidate Checks (A33)",
        "Kiem tra 3 chot chan cua Guard: (1) kieu tra ve cua guard phai la bool, "
        "(2) gia tri hien tai phai thoa guard ngay luc dang ky, (3) gia tri ung vien phai thoa guard truoc khi gan.",
        "Thu dang ky guard tra ve int (loi), dang ky guard tren bien dang am (loi), "
        "dang ky guard hop le `age >= 0 && age <= 150` va thu gan gia tri hop le / vi pham.",
        [] {
            Environment env;
            env.define("age", Value::Int(20));
            env.define("badStart", Value::Int(-5));

            // 4a. Non-boolean typed guard callback is rejected
            assert(throwsRuntimeError([&] {
                env.addTypedGuard("age", [](const Value& v) -> Value {
                    return Value::Int(v.asInt() + 1);
                });
            }));
            assert(!env.hasGuard("age"));

            // 4b. Non-boolean AST expression guard is rejected
            auto nonBoolExpr = parseExpr("age + 10");
            assert(throwsRuntimeError([&] {
                env.addExpressionGuard(*nonBoolExpr);
            }));

            // 4c. Initial value violating guard is rejected immediately
            assert(throwsRuntimeError([&] {
                env.addGuard("badStart", [](const Value& v) {
                    return v.asInt() >= 0;
                });
            }));
            assert(!env.hasGuard("badStart"));

            auto failingInitExpr = parseExpr("badStart >= 0");
            assert(throwsRuntimeError([&] {
                env.addExpressionGuard(*failingInitExpr);
            }));

            // 4d. Valid guard registration and candidate assignment enforcement
            auto ageGuardExpr = parseExpr("age >= 0 && age <= 150");
            env.addExpressionGuard(*ageGuardExpr);
            assert(env.hasGuard("age"));

            env.assign("age", Value::Int(35));
            assert(env.get("age").asInt() == 35);

            // Candidate violating guard is rejected and leaves live value unchanged
            assert(throwsRuntimeError([&] { env.assign("age", Value::Int(-1)); }));
            assert(env.get("age").asInt() == 35);

            assert(throwsRuntimeError([&] { env.assign("age", Value::Int(200)); }));
            assert(env.get("age").asInt() == 35);
        },
        passed);

    // Test 5: Multi-variable guards and list element guards
    runTest(
        5,
        "Multi-Variable AST Guards & List Element Guards (A46)",
        "Kiem tra guard rang buoc giua nhieu bien (`minVal <= maxVal`) "
        "va guard tren phan tu cua list (`scores[0] >= 0`).",
        "Parse bieu thuc AST, dang ky vao Environment va thu thay doi tung bien "
        "hoac phan tu list sao cho vi pham rang buoc chung.",
        [] {
            Environment env;
            env.define("minVal", Value::Int(10));
            env.define("maxVal", Value::Int(50));

            auto rangeExpr = parseExpr("minVal <= maxVal");
            env.addExpressionGuard(*rangeExpr);

            env.assign("minVal", Value::Int(40));
            assert(env.get("minVal").asInt() == 40);

            // Setting minVal > maxVal is rejected
            assert(throwsRuntimeError([&] { env.assign("minVal", Value::Int(60)); }));
            assert(env.get("minVal").asInt() == 40);

            // Setting maxVal < minVal is also rejected
            assert(throwsRuntimeError([&] { env.assign("maxVal", Value::Int(20)); }));
            assert(env.get("maxVal").asInt() == 50);

            // Guard on a list element: scores[0] >= 0
            env.define("scores", Value::List(Type::Int(), 2));
            env.assignListElement("scores", 0, Value::Int(85));
            auto listGuard = parseExpr("scores[0] >= 0");
            env.addExpressionGuard(*listGuard);

            env.assignListElement("scores", 0, Value::Int(95));
            assert(env.getListElement("scores", 0).asInt() == 95);

            assert(throwsRuntimeError([&] {
                env.assignListElement("scores", 0, Value::Int(-10));
            }));
            assert(env.getListElement("scores", 0).asInt() == 95);
        },
        passed);

    // Test 6: Integration of real Environment, Locks, Guards, and CheckpointManager (A18, A34-A37)
    runTest(
        6,
        "Atomic Checkpoint & Restore with Locks, Guards & Lists (A18, A34-A37)",
        "Kiem tra luu snapshot deep-copy (bao gom list elements & capacity) va "
        "khoi phuc nguyen tu: neu 1 bien bi lock hoac vi pham guard thi huy toan bo restore.",
        "Luu checkpoint cp1(x, y, items), thay doi gia tri, thu restore khi x bi lock "
        "hoac khi y bi chan boi guard `y >= 0`, xac nhan khong bien nao bi thay doi do dang.",
        [] {
            Environment env;
            CheckpointManager checkpoints;

            env.define("x", Value::Int(10));
            env.define("y", Value::Int(-5));
            env.define("items", Value::List(Type::Int(), 2, 8));
            env.assignListElement("items", 0, Value::Int(1));
            env.assignListElement("items", 1, Value::Int(2));

            checkpoints.save("cp1", {"x", "y", "items"}, env);

            env.assign("x", Value::Int(100));
            env.assign("y", Value::Int(200));
            env.assignListElement("items", 0, Value::Int(99));
            env.extendList("items", 4);

            // Lock 'x' -> atomic restore must fail without changing 'y' or 'items'
            env.lock("x");
            assert(throwsRuntimeError([&] { checkpoints.restore("cp1", env); }));
            assert(env.get("x").asInt() == 100);
            assert(env.get("y").asInt() == 200);
            assert(env.getListElement("items", 0).asInt() == 99);
            assert(env.get("items").asList().size() == 4);
            env.unlock("x");

            // Attach a guard `y >= 0` in inner scope -> restore cp1 (y == -5) must fail atomically
            env.pushScope();
            auto nonNegY = parseExpr("y >= 0");
            env.addExpressionGuard(*nonNegY);
            assert(throwsRuntimeError([&] { checkpoints.restore("cp1", env); }));
            assert(env.get("x").asInt() == 100);
            assert(env.get("y").asInt() == 200);
            assert(env.getListElement("items", 0).asInt() == 99);

            env.popScope();

            // After inner scope guard is popped, restore succeeds completely
            checkpoints.restore("cp1", env);
            assert(env.get("x").asInt() == 10);
            assert(env.get("y").asInt() == -5);
            assert(env.get("items").asList().size() == 2);
            assert(env.get("items").asList().declaredCapacity() == 8);
            assert(env.getListElement("items", 0).asInt() == 1);
            assert(env.getListElement("items", 1).asInt() == 2);
        },
        passed);

    // Test 7: Assumptions A38 & A41 - Shadowed bindings, expired scopes, and redeclared variables
    runTest(
        7,
        "Scope Identity & Binding Identity Verification on Restore (A38, A41, A43)",
        "Dam bao restore that bai neu bien goc bi che khuat (shadowed) o scope con, "
        "hoac bi xoa va khai bao lai (redeclared) trong cung 1 scope.",
        "Luu checkpoint cho x, pushScope() khai bao x moi (restore phai loi), "
        "popScope() roi undefine(x) + define(x) lai trong cung scope (restore phai loi do khac bindingId).",
        [] {
            Environment env;
            CheckpointManager checkpoints;

            env.define("x", Value::Int(10));
            checkpoints.save("outerCp", {"x"}, env);

            // 7a. Shadowing 'x' in an inner scope causes restore of 'outerCp' to fail
            env.pushScope();
            env.define("x", Value::Int(99));
            assert(throwsRuntimeError([&] { checkpoints.restore("outerCp", env); }));
            assert(env.get("x").asInt() == 99);
            env.popScope();

            // Once inner scope exits, original 'x' is exposed again and restore succeeds
            env.assign("x", Value::Int(55));
            checkpoints.restore("outerCp", env);
            assert(env.get("x").asInt() == 10);

            // 7b. Redeclaring 'x' in the SAME scope changes bindingId -> restore must fail!
            env.undefine("x");
            env.define("x", Value::Int(77));
            assert(throwsRuntimeError([&] { checkpoints.restore("outerCp", env); }));
            assert(env.get("x").asInt() == 77);
        },
        passed);

    // Test 8: Assumption A39 & A53 - C++ reference binding and RAII ScopeGuard cleanup
    runTest(
        8,
        "Generated C++ Variable Binding (A39) & RAII ScopeGuard Cleanup (A53)",
        "Kiem tra dong bo hai chieu giua bien C++ that va Environment (A39), "
        "va tu dong don dep scope/guard bang RAII ScopeGuard khi co ngoai le (A53).",
        "Goi bindReference() voi bien C++ int va std::string, thu assign() va restore(); "
        "dung ScopeGuard trong khoi try/catch nem exception de xac nhan scope tu dong pop.",
        [] {
            Environment env;
            CheckpointManager checkpoints;

            int cppCounter = 42;
            std::string cppLabel = "init";
            env.bindReference("counter", cppCounter);
            env.bindReference("label", cppLabel);

            checkpoints.save("cppState", {"counter", "label"}, env);

            env.assign("counter", Value::Int(100));
            env.assign("label", Value::String("changed"));
            assert(cppCounter == 100);
            assert(cppLabel == "changed");

            checkpoints.restore("cppState", env);
            assert(cppCounter == 42);
            assert(cppLabel == "init");

            try {
                ScopeGuard raiiScope(env);
                assert(env.scopeDepth() == 2);
                env.define("temp", Value::Int(5));
                env.addGuard("temp", [](const Value& v) { return v.asInt() > 0; });
                throw std::runtime_error("Simulated block exception");
            } catch (const std::runtime_error&) {
                assert(env.scopeDepth() == 1);
                assert(!env.hasVariable("temp"));
            }
        },
        passed);

    // Test 9: Assumptions A15, A48, A49 - Preview snapshot provider and error when unregistered
    runTest(
        9,
        "Built-in preview() Snapshot Provider & Error Handling (A15, A48, A49)",
        "Dam bao preview() nem runtime_error khi chua dang ky provider (A49) "
        "va xuat day du name, type, value, lock status, scope khi da dang ky (A15, A48).",
        "Goi previewActiveVariables() khi chua dang ky (phai nem loi), sau do "
        "registerPreviewEnvironment() va kiem tra chuoi ket qua cung createPreviewProvider().",
        [] {
            clearPreviewEnvironment();
            std::ostringstream out;

            // Calling preview without a registered provider must throw a runtime error (A49)
            assert(throwsRuntimeError([&] { previewActiveVariables(out); }));

            Environment env;
            env.define("count", Value::Int(7));
            env.lock("count");
            registerPreviewEnvironment(&env);

            previewActiveVariables(out);
            const std::string rendered = out.str();
            assert(rendered.find("count : int = 7 [locked] scope=global") != std::string::npos);

            auto provider = env.createPreviewProvider();
            auto snap = provider();
            assert(snap.size() == 1);
            assert(snap[0].name == "count");
            assert(snap[0].type == "int");
            assert(snap[0].value == "7");
            assert(snap[0].locked);
            assert(snap[0].scope == "global");

            clearPreviewEnvironment();
        },
        passed);

    std::cout << "============================================================\n";
    std::cout << "KET QUA: " << passed << "/" << total
              << " bai test Runtime Environment, Locks & Guards da PASS!\n";
    std::cout << "============================================================\n";
    return 0;
}

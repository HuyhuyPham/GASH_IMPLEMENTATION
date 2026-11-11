
#include <cassert>
#include <iostream>
#include <stdexcept>

#if __has_include("../runtime/Checkpoint.h")
#include "../runtime/Checkpoint.h"
#include "../runtime/Environment.h"
#include "../runtime/Value.h"
#else
#include "Checkpoint.h"
#include "Environment.h"
#include "Value.h"
#endif

using namespace gash;

// Check whether an operation throws a runtime error.
template <typename F>
void expectRuntimeError(F&& operation) {
    bool failed = false;

    try {
        operation();
    } catch (const std::runtime_error&) {
        failed = true;
    }

    assert(failed);
}

int main() {

    // Test 1: Save and restore real integer variables.
    {
        Environment env;
        CheckpointManager manager;

        env.define("x", Value::Int(10));
        env.define("y", Value::Int(20));

        manager.save("start", {"x", "y"}, env);

        env.assign("x", Value::Int(100));
        env.assign("y", Value::Int(200));

        manager.restore("start", env);

        assert(env.get("x").asInt() == 10);
        assert(env.get("y").asInt() == 20);

        // A checkpoint can be restored more than once.
        env.assign("x", Value::Int(99));
        manager.restore("start", env);

        assert(env.get("x").asInt() == 10);
    }

    // Test 2: Deep-copy list values and metadata.
    {
        Environment env;
        CheckpointManager manager;

        env.define("items", Value::List(Type::Int(), 3, 8));
        env.assignListElement("items", 0, Value::Int(7));

        manager.save("listBackup", {"items"}, env);

        env.assignListElement("items", 0, Value::Int(99));
        env.assignListElement("items", 1, Value::Int(50));
        env.extendList("items", 5);

        manager.restore("listBackup", env);

        const auto& list = env.get("items").asList();

        assert(list.size() == 3);
        assert(list.declaredCapacity() == 8);
        assert(list.get(0).asInt() == 7);
        assert(!list.isInitialized(1));
    }

    // Test 3: Locked variables prevent atomic restore.
    {
        Environment env;
        CheckpointManager manager;

        env.define("x", Value::Int(10));
        env.define("y", Value::Int(20));

        manager.save("locked", {"x", "y"}, env);

        env.assign("x", Value::Int(100));
        env.assign("y", Value::Int(200));
        env.lock("y");

        expectRuntimeError([&] {
            manager.restore("locked", env);
        });

        assert(env.get("x").asInt() == 100);
        assert(env.get("y").asInt() == 200);
    }

    // Test 4: Guard failure prevents atomic restore.
    {
        Environment env;
        CheckpointManager manager;

        env.define("x", Value::Int(10));
        env.define("y", Value::Int(-5));

        manager.save("guardBackup", {"x", "y"}, env);

        env.assign("x", Value::Int(100));
        env.assign("y", Value::Int(200));

        // The current value is valid, but the saved value is not.
        env.addGuard("y", [](const Value& value) {
            return value.asInt() >= 0;
        });

        expectRuntimeError([&] {
            manager.restore("guardBackup", env);
        });

        assert(env.get("x").asInt() == 100);
        assert(env.get("y").asInt() == 200);
    }

    // Test 5: Reject a different binding with the same name.
    {
        Environment env;
        CheckpointManager manager;

        env.define("x", Value::Int(10));
        manager.save("original", {"x"}, env);

        env.undefine("x");
        env.define("x", Value::Int(30));

        expectRuntimeError([&] {
            manager.restore("original", env);
        });

        assert(env.get("x").asInt() == 30);
    }

    // Test 6: Reject shadowed variables in another scope.
    {
        Environment env;
        CheckpointManager manager;

        env.define("x", Value::Int(10));
        manager.save("outer", {"x"}, env);

        env.assign("x", Value::Int(15));

        env.pushScope();
        env.define("x", Value::Int(99));

        expectRuntimeError([&] {
            manager.restore("outer", env);
        });

        assert(env.get("x").asInt() == 99);

        env.popScope();

        manager.restore("outer", env);
        assert(env.get("x").asInt() == 10);
    }

    // Test 7: Restore a bound C++ variable.
    {
        Environment env;
        CheckpointManager manager;

        int cppX = 10;

        env.bindReference("x", cppX);
        manager.save("cppBackup", {"x"}, env);

        env.assign("x", Value::Int(90));
        assert(cppX == 90);

        manager.restore("cppBackup", env);

        assert(cppX == 10);
        assert(env.get("x").asInt() == 10);
    }

    std::cout << "Real Runtime checkpoint tests passed!\n";
    return 0;
}
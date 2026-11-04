#include <any>
#include <cassert>
#include <iostream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <variant>
#include <vector>

#include "Checkpoint.h"

using namespace gash;

namespace {

// A test-only value. std::vector is owned, so copying this is a deep copy.
struct ListState {
  std::vector<int> elements;
  std::size_t declaredCapacity = 0;
};
using FakeValue = std::variant<int, ListState>;

// Test double: Hihi's actual Environment can implement the same contract.
class FakeEnvironment final : public CheckpointEnvironment {
 public:
  void defineInt(const std::string& name, int value, std::size_t scope = 1) {
    bindings_[name] = Binding{FakeValue{value}, "int", scope, true};
  }

  void defineList(const std::string& name, std::vector<int> elements,
                  std::size_t capacity, std::size_t scope = 1) {
    bindings_[name] =
        Binding{FakeValue{ListState{std::move(elements), capacity}},
                "list<int>", scope, true};
  }

  void setInt(const std::string& name, int value) {
    std::get<int>(bindings_.at(name).value) = value;
  }

  int getInt(const std::string& name) const {
    return std::get<int>(bindings_.at(name).value);
  }

  void setList(const std::string& name, std::vector<int> elements,
               std::size_t capacity) {
    bindings_.at(name).value = ListState{std::move(elements), capacity};
  }

  const ListState& getList(const std::string& name) const {
    return std::get<ListState>(bindings_.at(name).value);
  }

  void setScope(const std::string& name, std::size_t scope) {
    bindings_.at(name).scopeId = scope;
  }

  void changeType(const std::string& name, const std::string& type) {
    bindings_.at(name).type = type;
  }

  void erase(const std::string& name) { bindings_.erase(name); }

  void lock(const std::string& name) { locked_.insert(name); }
  void guardNonnegative(const std::string& name) { guarded_.insert(name); }

  std::vector<VariableSnapshot> captureVariables(
      const std::vector<std::string>& names) const override {
    std::vector<VariableSnapshot> result;
    result.reserve(names.size());

    for (const auto& name : names) {
      const auto it = bindings_.find(name);
      if (it == bindings_.end()) {
        throw std::runtime_error("Unknown variable: " + name);
      }
      const Binding& b = it->second;
      result.push_back(VariableSnapshot{
          name,
          std::any{b.value},  // Owns a full copy, including list elements.
          b.type, b.scopeId, b.initialized});
    }
    return result;
  }

  void restoreVariablesAtomically(
      const std::vector<VariableSnapshot>& variables) override {
    // Stage a copy of the entire environment (strong exception safety).
    auto candidate = bindings_;

    for (const auto& snapshot : variables) {
      auto it = candidate.find(snapshot.name);
      if (it == candidate.end()) {
        throw std::runtime_error("Unknown variable on restore");
      }
      if (it->second.scopeId != snapshot.scopeId) {
        throw std::runtime_error("Scope mismatch on restore");
      }
      if (it->second.type != snapshot.declaredType) {
        throw std::runtime_error("Type mismatch on restore");
      }
      if (locked_.count(snapshot.name)) {
        throw std::runtime_error("Locked variable on restore");
      }

      const auto* value = std::any_cast<FakeValue>(&snapshot.value);
      if (value == nullptr) {
        throw std::runtime_error("Invalid snapshot value");
      }
      if (guarded_.count(snapshot.name)) {
        const auto* intValue = std::get_if<int>(value);
        if (intValue != nullptr && *intValue < 0) {
          throw std::runtime_error("Guard rejected restore");
        }
      }

      it->second.value = *value;
      it->second.initialized = snapshot.initialized;
    }

    // Nothing in the live environment changes until this single commit.
    bindings_.swap(candidate);
  }

 private:
  struct Binding {
    FakeValue value;
    std::string type;
    std::size_t scopeId = 0;
    bool initialized = false;
  };

  std::unordered_map<std::string, Binding> bindings_;
  std::unordered_set<std::string> locked_;
  std::unordered_set<std::string> guarded_;
};

template <typename F>
bool throwsRuntimeError(F&& fn) {
  try {
    fn();
  } catch (const std::runtime_error&) {
    return true;
  }
  return false;
}

}  // namespace

int main() {
  // Test 1: Save and restore two integer variables.
  {
    FakeEnvironment env;
    env.defineInt("x", 10);
    env.defineInt("y", 20);
    CheckpointManager manager;
    manager.save("start", {"x", "y"}, env);
    env.setInt("x", 100);
    env.setInt("y", 200);
    manager.restore("start", env);
    assert(env.getInt("x") == 10);
    assert(env.getInt("y") == 20);

    // The checkpoint remains valid for repeated restores.
    env.setInt("x", 999);
    manager.restore("start", env);
    assert(env.getInt("x") == 10);
  }

  // Test 2: Snapshot owns deep copies of lists and capacity metadata.
  {
    FakeEnvironment env;
    env.defineList("items", {1, 2, 3}, 8);
    CheckpointManager manager;
    manager.save("listBackup", {"items"}, env);
    env.setList("items", {7, 8, 9, 10}, 20);
    manager.restore("listBackup", env);
    assert((env.getList("items").elements == std::vector<int>{1, 2, 3}));
    assert(env.getList("items").declaredCapacity == 8);
  }

  // Test 3: Missing checkpoint is rejected.
  {
    FakeEnvironment env;
    CheckpointManager manager;
    assert(throwsRuntimeError([&] { manager.restore("missing", env); }));
  }

  // Test 4: Duplicate labels / variable names and missing variables fail.
  {
    FakeEnvironment env;
    env.defineInt("x", 5);
    CheckpointManager manager;
    manager.save("one", {"x"}, env);
    assert(throwsRuntimeError([&] { manager.save("one", {"x"}, env); }));
    assert(throwsRuntimeError([&] { manager.save("dup", {"x", "x"}, env); }));
    assert(throwsRuntimeError([&] { manager.save("missing", {"z"}, env); }));
    assert(throwsRuntimeError([&] { manager.save("empty", {}, env); }));
    assert(throwsRuntimeError([&] { manager.save("", {"x"}, env); }));
  }

  // Test 5: One locked variable rejects the whole restore (atomic).
  {
    FakeEnvironment env;
    env.defineInt("x", 10);
    env.defineInt("y", 20);
    CheckpointManager manager;
    manager.save("locked", {"x", "y"}, env);
    env.setInt("x", 100);
    env.setInt("y", 200);
    env.lock("y");
    assert(throwsRuntimeError([&] { manager.restore("locked", env); }));
    assert(env.getInt("x") == 100);
    assert(env.getInt("y") == 200);
  }

  // Test 6: Scope mismatch rejects the whole restore.
  {
    FakeEnvironment env;
    env.defineInt("x", 10);
    env.defineInt("y", 20);
    CheckpointManager manager;
    manager.save("scope", {"x", "y"}, env);
    env.setInt("x", 100);
    env.setInt("y", 200);
    env.setScope("y", 2);
    assert(throwsRuntimeError([&] { manager.restore("scope", env); }));
    assert(env.getInt("x") == 100);
    assert(env.getInt("y") == 200);
  }

  // Test 7: Guard failure rejects the whole restore.
  {
    FakeEnvironment env;
    env.defineInt("x", 10);
    env.defineInt("y", -5);
    CheckpointManager manager;
    manager.save("guard", {"x", "y"}, env);
    env.setInt("x", 100);
    env.setInt("y", 200);
    env.guardNonnegative("y");
    assert(throwsRuntimeError([&] { manager.restore("guard", env); }));
    assert(env.getInt("x") == 100);
    assert(env.getInt("y") == 200);
  }

  // Test 8: Type mismatch rejects a restore without changing variables.
  {
    FakeEnvironment env;
    env.defineInt("x", 10);
    env.defineInt("y", 20);
    CheckpointManager manager;
    manager.save("types", {"x", "y"}, env);
    env.setInt("x", 100);
    env.setInt("y", 200);
    env.changeType("y", "float");
    assert(throwsRuntimeError([&] { manager.restore("types", env); }));
    assert(env.getInt("x") == 100);
    assert(env.getInt("y") == 200);
  }

  // Test 9: Missing variable rejects a restore without partial changes.
  {
    FakeEnvironment env;
    env.defineInt("x", 10);
    env.defineInt("y", 20);
    CheckpointManager manager;
    manager.save("deleted", {"x", "y"}, env);
    env.setInt("x", 100);
    env.erase("y");
    assert(throwsRuntimeError([&] { manager.restore("deleted", env); }));
    assert(env.getInt("x") == 100);
  }

  std::cout << "Checkpoint helper tests passed!\n";
  return 0;
}
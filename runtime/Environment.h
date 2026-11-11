#ifndef GASH_ENVIRONMENT_H
#define GASH_ENVIRONMENT_H

#include "Checkpoint.h"
#include "Guard.h"
#include "List.h"
#if __has_include("../include/Type.h")
#include "../include/Type.h"
#else
#include "Type.h"
#endif
#include "Value.h"

#include <cstddef>
#include <cstdint>
#include <functional>
#include <iostream>
#include <ostream>
#include <string>
#include <unordered_map>
#include <vector>

namespace gash {

// Active variable metadata exported for preview() (Assumptions A15, A48).
struct PreviewVariableInfo {
    std::string name;
    std::string type;
    std::string value;
    bool locked = false;
    std::string scope;
};

// Represents a single live variable binding in a runtime scope frame.
struct VariableBinding {
    std::string name;
    Type type = Type::Unknown();
    Value value;
    std::size_t scopeId = 0;
    std::size_t bindingId = 0;
    bool initialized = false;
    bool locked = false;
    std::function<void(const Value&)> writeBackHook;
    std::function<Value()> readThroughHook;
};

// Runtime execution environment managing variable state, lexical scope frames,
// central assignment validation, lock/unlock permissions, guards, and checkpoints.
class Environment : public CheckpointEnvironment {
public:
    Environment();
    ~Environment() override = default;

    // Scope management
    std::size_t pushScope();
    void popScope();
    std::size_t scopeDepth() const noexcept { return frames_.size(); }
    std::size_t currentScopeId() const noexcept { return frames_.back().id; }

    // Variable declarations in the current innermost scope
    void define(const std::string& name, const Value& initialValue);
    void define(
        const std::string& name,
        const Type& declaredType,
        const Value& initialValue
    );
    void defineUninitialized(const std::string& name, const Type& declaredType);

    // Bind a generated C++ variable or list reference to the Environment (Assumption A39).
    void bindReference(const std::string& name, int& cppVar);
    void bindReference(const std::string& name, double& cppVar);
    void bindReference(const std::string& name, bool& cppVar);
    void bindReference(const std::string& name, char& cppVar);
    void bindReference(const std::string& name, std::string& cppVar);
    void bindReference(const std::string& name, List<int>& cppList);
    void bindReference(const std::string& name, List<double>& cppList);
    void bindReference(const std::string& name, List<bool>& cppList);
    void bindReference(const std::string& name, List<char>& cppList);
    void bindReference(const std::string& name, List<std::string>& cppList);

    // Remove a variable from its scope (used for testing redeclarations / A38 / A41).
    void undefine(const std::string& name);

    // Variable queries (searches from innermost scope outward)
    bool hasVariable(const std::string& name) const;
    bool hasVariableInCurrentScope(const std::string& name) const;
    const Value& get(const std::string& name) const;
    const VariableBinding& getBinding(const std::string& name) const;
    Type getType(const std::string& name) const;
    std::size_t getScopeId(const std::string& name) const;
    std::size_t getBindingId(const std::string& name) const;
    bool isInitialized(const std::string& name) const;

    // Central assignment and mutation API (enforces types, locks, and guards)
    void assign(const std::string& name, const Value& newValue);

    // List element read/write and resize through the central mutation API
    Value getListElement(const std::string& name, std::int64_t index) const;
    void assignListElement(
        const std::string& name,
        std::int64_t index,
        const Value& elementValue
    );
    void extendList(const std::string& name, std::int64_t newSize);

    // Generated C++ mutation wrappers (enforce locks and guards atomically with rollback)
    template <typename Func>
    auto mutateVar(const std::string& name, Func&& fn) -> decltype(fn()) {
        beginMutation(name, false);
        Value oldVal = snapshotCurrentValue(name);
        try {
            auto result = fn();
            commitOrRollbackMutation(name, oldVal);
            return result;
        } catch (...) {
            rollbackValue(name, oldVal);
            throw;
        }
    }

    template <typename Func>
    auto mutateList(const std::string& name, Func&& fn) -> decltype(fn()) {
        beginMutation(name, true);
        Value oldVal = snapshotCurrentValue(name);
        try {
            auto result = fn();
            commitOrRollbackMutation(name, oldVal);
            return result;
        } catch (...) {
            rollbackValue(name, oldVal);
            throw;
        }
    }

    template <typename Func>
    void mutateListVoid(const std::string& name, Func&& fn) {
        beginMutation(name, true);
        Value oldVal = snapshotCurrentValue(name);
        try {
            fn();
            commitOrRollbackMutation(name, oldVal);
        } catch (...) {
            rollbackValue(name, oldVal);
            throw;
        }
    }

    // Lock and unlock controls (repeated lock/unlock is a safe no-op)
    void lock(const std::string& name);
    void unlock(const std::string& name);
    bool isLocked(const std::string& name) const;

    // Mutation guard registration and queries
    void addGuard(
        const std::string& name,
        GuardPredicate predicate,
        std::string description = ""
    );
    void addTypedGuard(
        const std::string& name,
        TypedGuardCallback evaluator,
        std::string description = ""
    );
    void addExpressionGuard(const Expr& condition);
    void addLiveGuard(
        std::function<bool()> check,
        std::string description = ""
    );
    bool hasGuard(const std::string& name) const;

    const GuardManager& guards() const noexcept { return guards_; }
    GuardManager& guards() noexcept { return guards_; }

    // Preview snapshot export and callback adapter (Assumptions A15, A48, A49).
    std::vector<PreviewVariableInfo> exportActiveVariables() const;

    template <typename SnapshotT = PreviewVariableInfo>
    std::function<std::vector<SnapshotT>()> createPreviewProvider() const {
        return [this]() {
            auto active = this->exportActiveVariables();
            std::vector<SnapshotT> out;
            out.reserve(active.size());
            for (const auto& item : active) {
                out.push_back(SnapshotT{
                    item.name,
                    item.type,
                    item.value,
                    item.locked,
                    item.scope
                });
            }
            return out;
        };
    }

    // CheckpointEnvironment interface implementation
    std::vector<VariableSnapshot> captureVariables(
        const std::vector<std::string>& names
    ) const override;

    void restoreVariablesAtomically(
        const std::vector<VariableSnapshot>& variables
    ) override;

private:
    struct ScopeFrame {
        std::size_t id = 0;
        std::vector<std::string> declarationOrder;
        std::unordered_map<std::string, VariableBinding> variables;
    };

    struct LiveGuardEntry {
        std::size_t registrationScopeId = 0;
        std::function<bool()> check;
        std::string description;
    };

    std::vector<ScopeFrame> frames_;
    std::size_t nextScopeId_ = 1;
    std::size_t nextBindingId_ = 1;
    GuardManager guards_;
    std::vector<LiveGuardEntry> liveGuards_;

    const VariableBinding* findBinding(const std::string& name) const;
    VariableBinding* findBindingMutable(const std::string& name);

    static const VariableBinding* findBindingInFrames(
        const std::vector<ScopeFrame>& frames,
        const std::string& name
    );
    static VariableBinding* findBindingInFramesMutable(
        std::vector<ScopeFrame>& frames,
        const std::string& name
    );

    VariableResolver makeResolver() const;
    static VariableResolver makeResolverForFrames(
        const std::vector<ScopeFrame>& frames
    );

    void beginMutation(const std::string& name, bool isListOp) const;
    Value snapshotCurrentValue(const std::string& name) const;
    void commitOrRollbackMutation(const std::string& name, const Value& oldVal);
    void rollbackValue(const std::string& name, const Value& oldVal);
    void validateLiveGuards() const;
};

// RAII scope guard ensuring automatic scope cleanup on normal exit or exception unwinding (Assumption A53).
class ScopeGuard {
public:
    explicit ScopeGuard(Environment& env) : env_(env) {
        env_.pushScope();
    }

    ~ScopeGuard() noexcept {
        try {
            if (active_) {
                env_.popScope();
            }
        } catch (...) {
        }
    }

    ScopeGuard(const ScopeGuard&) = delete;
    ScopeGuard& operator=(const ScopeGuard&) = delete;

private:
    Environment& env_;
    bool active_ = true;
};

// Global preview provider hooks (Assumptions A48, A49).
void registerPreviewEnvironment(const Environment* env);
void clearPreviewEnvironment();
void previewActiveVariables(std::ostream& out = std::cout);

} // namespace gash

#endif // GASH_ENVIRONMENT_H

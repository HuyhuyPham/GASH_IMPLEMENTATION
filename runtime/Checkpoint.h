#ifndef GASH_CHECKPOINT_H
#define GASH_CHECKPOINT_H

#include <any>
#include <cstddef>
#include <memory>
#include <string>
#include <vector>

namespace gash {

// An owning copy of one variable's state at checkpoint time.
// The environment must put a DEEP COPY of its Value in 'value'.
struct VariableSnapshot {
    std::string name;
    std::any value;
    std::string declaredType;
    std::size_t scopeId = 0;
    bool initialized = false;
};

// A small adapter interface between G5 and Hihi's actual Environment.
// Hihi can implement this interface directly or through an adapter.
class CheckpointEnvironment {
public:
    virtual ~CheckpointEnvironment() = default;

    // Must return an owning, deep-copied snapshot for each requested name,
    // in the same order, or throw without changing the environment.
    virtual std::vector<VariableSnapshot> captureVariables(
        const std::vector<std::string>& names
    ) const = 0;

    // Must validate all snapshots (scope, types, locks, guards, etc.) BEFORE
    // applying anything. On failure, the environment must remain unchanged.
    virtual void restoreVariablesAtomically(
        const std::vector<VariableSnapshot>& variables
    ) = 0;
};

class CheckpointManager {
public:
    CheckpointManager();
    ~CheckpointManager();

    CheckpointManager(const CheckpointManager&) = delete;
    CheckpointManager& operator=(const CheckpointManager&) = delete;

    // Save selected variables under a unique checkpoint label.
    void save(
        const std::string& label,
        const std::vector<std::string>& variableNames,
        const CheckpointEnvironment& environment
    );

    // Restore a saved checkpoint. The environment guarantees atomicity.
    void restore(
        const std::string& label,
        CheckpointEnvironment& environment
    );

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace gash

#endif // GASH_CHECKPOINT_H
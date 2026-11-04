#include "Checkpoint.h"

#include <stdexcept>
#include <unordered_map>
#include <unordered_set>
#include <utility>

namespace gash {

struct CheckpointManager::Impl {
    struct Snapshot {
        std::vector<VariableSnapshot> variables;
    };

    std::unordered_map<std::string, Snapshot> snapshots;
};

CheckpointManager::CheckpointManager()
    : impl_(std::make_unique<Impl>()) {}

CheckpointManager::~CheckpointManager() = default;

void CheckpointManager::save(
    const std::string& label,
    const std::vector<std::string>& variableNames,
    const CheckpointEnvironment& environment
) {
    if (label.empty()) {
        throw std::runtime_error("Checkpoint label cannot be empty");
    }
    if (variableNames.empty()) {
        throw std::runtime_error("Checkpoint requires at least one variable");
    }
    if (impl_->snapshots.find(label) != impl_->snapshots.end()) {
        throw std::runtime_error("Checkpoint already exists: " + label);
    }

    std::unordered_set<std::string> seen;
    for (const auto& name : variableNames) {
        if (name.empty()) {
            throw std::runtime_error("Checkpoint variable name cannot be empty");
        }
        if (!seen.insert(name).second) {
            throw std::runtime_error("Duplicate checkpoint variable: " + name);
        }
    }

    // Capture first: if Environment throws, no checkpoint is added.
    auto captured = environment.captureVariables(variableNames);

    if (captured.size() != variableNames.size()) {
        throw std::runtime_error("Incomplete checkpoint snapshot");
    }
    for (std::size_t i = 0; i < captured.size(); ++i) {
        if (captured[i].name != variableNames[i]) {
            throw std::runtime_error("Checkpoint variable mismatch");
        }
    }

    // Commit the complete snapshot only after all checks succeed.
    impl_->snapshots.emplace(label, Impl::Snapshot{std::move(captured)});
}

void CheckpointManager::restore(
    const std::string& label,
    CheckpointEnvironment& environment
) {
    const auto it = impl_->snapshots.find(label);
    if (it == impl_->snapshots.end()) {
        throw std::runtime_error("Unknown checkpoint: " + label);
    }

    // The environment is responsible for validating and committing atomically.
    // Do not change variables one at a time here.
    environment.restoreVariablesAtomically(it->second.variables);
}

} // namespace gash
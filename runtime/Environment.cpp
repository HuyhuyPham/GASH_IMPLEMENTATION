#include "Environment.h"

#include <algorithm>
#include <stdexcept>
#include <utility>

namespace gash {

namespace {

const Environment*& activePreviewEnvironmentStorage() {
    static const Environment* env = nullptr;
    return env;
}

std::string formatScopeLabel(std::size_t scopeId) {
    if (scopeId == 1) {
        return "global";
    }
    return "scope#" + std::to_string(scopeId);
}

} // anonymous namespace

Environment::Environment() {
    // Initialize with global scope frame (scopeId = 1).
    pushScope();
}

std::size_t Environment::pushScope() {
    const std::size_t id = nextScopeId_++;
    frames_.push_back(ScopeFrame{id, {}, {}});
    return id;
}

void Environment::popScope() {
    if (frames_.size() <= 1) {
        throw std::runtime_error("Cannot pop global environment scope");
    }
    const std::size_t poppedId = frames_.back().id;
    guards_.removeGuardsForScope(poppedId);
    frames_.pop_back();
}

void Environment::define(const std::string& name, const Value& initialValue) {
    if (!initialValue.isInitialized()) {
        throw std::runtime_error(
            "Cannot infer type for uninitialized variable '" + name + "'");
    }
    define(name, initialValue.type(), initialValue);
}

void Environment::define(
    const std::string& name,
    const Type& declaredType,
    const Value& initialValue
) {
    if (name.empty()) {
        throw std::runtime_error("Variable name cannot be empty");
    }
    if (!declaredType.isValid() || declaredType.isVoid()) {
        throw std::runtime_error(
            "Invalid declared type '" + declaredType.toString() +
            "' for variable '" + name + "'");
    }
    if (!initialValue.isInitialized()) {
        throw std::runtime_error(
            "Initial value for variable '" + name + "' must be initialized");
    }
    if (initialValue.type() != declaredType) {
        throw std::runtime_error(
            "Type mismatch initializing variable '" + name + "': expected '" +
            declaredType.toString() + "', got '" +
            initialValue.type().toString() + "'");
    }

    auto& current = frames_.back();
    if (current.variables.find(name) != current.variables.end()) {
        throw std::runtime_error(
            "Variable '" + name + "' is already defined in the current scope");
    }

    const std::size_t bindingId = nextBindingId_++;
    VariableBinding binding;
    binding.name = name;
    binding.type = declaredType;
    binding.value = initialValue;
    binding.value.setBindingId(bindingId);
    binding.scopeId = current.id;
    binding.bindingId = bindingId;
    binding.initialized = true;
    binding.locked = false;

    current.declarationOrder.push_back(name);
    current.variables.emplace(name, std::move(binding));
}

void Environment::defineUninitialized(
    const std::string& name,
    const Type& declaredType
) {
    if (name.empty()) {
        throw std::runtime_error("Variable name cannot be empty");
    }
    if (!declaredType.isValid() || declaredType.isVoid()) {
        throw std::runtime_error(
            "Invalid declared type '" + declaredType.toString() +
            "' for variable '" + name + "'");
    }

    auto& current = frames_.back();
    if (current.variables.find(name) != current.variables.end()) {
        throw std::runtime_error(
            "Variable '" + name + "' is already defined in the current scope");
    }

    const std::size_t bindingId = nextBindingId_++;
    VariableBinding binding;
    binding.name = name;
    binding.type = declaredType;
    binding.value = Value();
    binding.value.setBindingId(bindingId);
    binding.scopeId = current.id;
    binding.bindingId = bindingId;
    binding.initialized = false;
    binding.locked = false;

    current.declarationOrder.push_back(name);
    current.variables.emplace(name, std::move(binding));
}

void Environment::bindReference(const std::string& name, int& cppVar) {
    if (!hasVariableInCurrentScope(name)) {
        define(name, Type::Int(), Value::Int(cppVar));
    }
    VariableBinding* b = findBindingMutable(name);
    if (!b || b->type != Type::Int()) {
        throw std::runtime_error("Cannot bind 'int&' to variable '" + name + "'");
    }
    b->writeBackHook = [&cppVar](const Value& v) { cppVar = v.asInt(); };
    b->readThroughHook = [&cppVar]() { return Value::Int(cppVar); };
}

void Environment::bindReference(const std::string& name, double& cppVar) {
    if (!hasVariableInCurrentScope(name)) {
        define(name, Type::Float(), Value::Float(cppVar));
    }
    VariableBinding* b = findBindingMutable(name);
    if (!b || b->type != Type::Float()) {
        throw std::runtime_error("Cannot bind 'double&' to variable '" + name + "'");
    }
    b->writeBackHook = [&cppVar](const Value& v) { cppVar = v.asFloat(); };
    b->readThroughHook = [&cppVar]() { return Value::Float(cppVar); };
}

void Environment::bindReference(const std::string& name, bool& cppVar) {
    if (!hasVariableInCurrentScope(name)) {
        define(name, Type::Bool(), Value::Bool(cppVar));
    }
    VariableBinding* b = findBindingMutable(name);
    if (!b || b->type != Type::Bool()) {
        throw std::runtime_error("Cannot bind 'bool&' to variable '" + name + "'");
    }
    b->writeBackHook = [&cppVar](const Value& v) { cppVar = v.asBool(); };
    b->readThroughHook = [&cppVar]() { return Value::Bool(cppVar); };
}

void Environment::bindReference(const std::string& name, char& cppVar) {
    if (!hasVariableInCurrentScope(name)) {
        define(name, Type::Char(), Value::Char(cppVar));
    }
    VariableBinding* b = findBindingMutable(name);
    if (!b || b->type != Type::Char()) {
        throw std::runtime_error("Cannot bind 'char&' to variable '" + name + "'");
    }
    b->writeBackHook = [&cppVar](const Value& v) { cppVar = v.asChar(); };
    b->readThroughHook = [&cppVar]() { return Value::Char(cppVar); };
}

void Environment::bindReference(const std::string& name, std::string& cppVar) {
    if (!hasVariableInCurrentScope(name)) {
        define(name, Type::String(), Value::String(cppVar));
    }
    VariableBinding* b = findBindingMutable(name);
    if (!b || b->type != Type::String()) {
        throw std::runtime_error("Cannot bind 'std::string&' to variable '" + name + "'");
    }
    b->writeBackHook = [&cppVar](const Value& v) { cppVar = v.asString(); };
    b->readThroughHook = [&cppVar]() { return Value::String(cppVar); };
}

void Environment::undefine(const std::string& name) {
    for (auto it = frames_.rbegin(); it != frames_.rend(); ++it) {
        auto found = it->variables.find(name);
        if (found != it->variables.end()) {
            it->variables.erase(found);
            it->declarationOrder.erase(
                std::remove(
                    it->declarationOrder.begin(),
                    it->declarationOrder.end(),
                    name),
                it->declarationOrder.end());
            return;
        }
    }
    throw std::runtime_error("Cannot undefine unknown variable: " + name);
}

const VariableBinding* Environment::findBindingInFrames(
    const std::vector<ScopeFrame>& frames,
    const std::string& name
) {
    for (auto it = frames.rbegin(); it != frames.rend(); ++it) {
        const auto found = it->variables.find(name);
        if (found != it->variables.end()) {
            return &found->second;
        }
    }
    return nullptr;
}

VariableBinding* Environment::findBindingInFramesMutable(
    std::vector<ScopeFrame>& frames,
    const std::string& name
) {
    for (auto it = frames.rbegin(); it != frames.rend(); ++it) {
        const auto found = it->variables.find(name);
        if (found != it->variables.end()) {
            return &found->second;
        }
    }
    return nullptr;
}

const VariableBinding* Environment::findBinding(const std::string& name) const {
    return findBindingInFrames(frames_, name);
}

VariableBinding* Environment::findBindingMutable(const std::string& name) {
    return findBindingInFramesMutable(frames_, name);
}

bool Environment::hasVariable(const std::string& name) const {
    return findBinding(name) != nullptr;
}

bool Environment::hasVariableInCurrentScope(const std::string& name) const {
    const auto& current = frames_.back();
    return current.variables.find(name) != current.variables.end();
}

const VariableBinding& Environment::getBinding(const std::string& name) const {
    const VariableBinding* binding = findBinding(name);
    if (!binding) {
        throw std::runtime_error("Undeclared variable: " + name);
    }
    return *binding;
}

const Value& Environment::get(const std::string& name) const {
    const VariableBinding& binding = getBinding(name);
    if (!binding.initialized || !binding.value.isInitialized()) {
        throw std::runtime_error("Read of uninitialized variable: " + name);
    }
    return binding.value;
}

Type Environment::getType(const std::string& name) const {
    return getBinding(name).type;
}

std::size_t Environment::getScopeId(const std::string& name) const {
    return getBinding(name).scopeId;
}

std::size_t Environment::getBindingId(const std::string& name) const {
    return getBinding(name).bindingId;
}

bool Environment::isInitialized(const std::string& name) const {
    return getBinding(name).initialized;
}

VariableResolver Environment::makeResolver() const {
    return [this](const std::string& name) -> Value {
        return this->get(name);
    };
}

VariableResolver Environment::makeResolverForFrames(
    const std::vector<ScopeFrame>& frames
) {
    return [&frames](const std::string& name) -> Value {
        const VariableBinding* b = findBindingInFrames(frames, name);
        if (!b) {
            throw std::runtime_error("Undeclared variable: " + name);
        }
        if (!b->initialized || !b->value.isInitialized()) {
            throw std::runtime_error("Read of uninitialized variable: " + name);
        }
        return b->value;
    };
}

void Environment::assign(const std::string& name, const Value& newValue) {
    VariableBinding* binding = findBindingMutable(name);
    if (!binding) {
        throw std::runtime_error("Cannot assign to undeclared variable: " + name);
    }
    if (binding->locked) {
        throw std::runtime_error("Cannot modify locked variable: " + name);
    }
    if (!newValue.isInitialized()) {
        throw std::runtime_error(
            "Cannot assign uninitialized value to variable: " + name);
    }
    if (newValue.type() != binding->type) {
        throw std::runtime_error(
            "Type mismatch in assignment to '" + name + "': expected '" +
            binding->type.toString() + "', got '" +
            newValue.type().toString() + "'");
    }

    // Validate candidate assignment against all active guards before mutating.
    guards_.validateCandidate(
        name,
        binding->scopeId,
        newValue,
        makeResolver());

    binding->value = newValue;
    binding->value.setBindingId(binding->bindingId);
    binding->initialized = true;
    if (binding->writeBackHook) {
        binding->writeBackHook(binding->value);
    }
}

Value Environment::getListElement(
    const std::string& name,
    std::int64_t index
) const {
    const Value& listVal = get(name);
    if (!listVal.isList()) {
        throw std::runtime_error("Variable '" + name + "' is not a list");
    }
    return listVal.asList().get(index);
}

void Environment::assignListElement(
    const std::string& name,
    std::int64_t index,
    const Value& elementValue
) {
    VariableBinding* binding = findBindingMutable(name);
    if (!binding) {
        throw std::runtime_error("Cannot index undeclared variable: " + name);
    }
    if (!binding->type.isList() || !binding->value.isList()) {
        throw std::runtime_error("Variable '" + name + "' is not a list");
    }
    if (binding->locked) {
        throw std::runtime_error(
            "Cannot modify element of locked list '" + name + "'");
    }

    // Stage a deep copy of the list and apply the element change on the candidate.
    ListValue candidateList = binding->value.asList();
    candidateList.set(index, elementValue);
    Value candidateValue(std::move(candidateList));
    candidateValue.setBindingId(binding->bindingId);

    // Validate guards against the candidate list state before committing.
    guards_.validateCandidate(
        name,
        binding->scopeId,
        candidateValue,
        makeResolver());

    binding->value = std::move(candidateValue);
    binding->initialized = true;
}

void Environment::extendList(const std::string& name, std::int64_t newSize) {
    VariableBinding* binding = findBindingMutable(name);
    if (!binding) {
        throw std::runtime_error("Cannot extend undeclared variable: " + name);
    }
    if (!binding->type.isList() || !binding->value.isList()) {
        throw std::runtime_error("Variable '" + name + "' is not a list");
    }
    if (binding->locked) {
        throw std::runtime_error("Cannot resize/extend locked list '" + name + "'");
    }

    ListValue candidateList = binding->value.asList();
    candidateList.resize(newSize);
    Value candidateValue(std::move(candidateList));
    candidateValue.setBindingId(binding->bindingId);

    guards_.validateCandidate(
        name,
        binding->scopeId,
        candidateValue,
        makeResolver());

    binding->value = std::move(candidateValue);
    binding->initialized = true;
}

void Environment::lock(const std::string& name) {
    VariableBinding* binding = findBindingMutable(name);
    if (!binding) {
        throw std::runtime_error("Cannot lock undeclared variable: " + name);
    }
    // Repeated lock is a safe no-op.
    binding->locked = true;
}

void Environment::unlock(const std::string& name) {
    VariableBinding* binding = findBindingMutable(name);
    if (!binding) {
        throw std::runtime_error("Cannot unlock undeclared variable: " + name);
    }
    // Repeated unlock is a safe no-op.
    binding->locked = false;
}

bool Environment::isLocked(const std::string& name) const {
    return getBinding(name).locked;
}

void Environment::addGuard(
    const std::string& name,
    GuardPredicate predicate,
    std::string description
) {
    const VariableBinding& binding = getBinding(name);
    if (!binding.initialized || !binding.value.isInitialized()) {
        throw std::runtime_error(
            "Cannot attach guard to uninitialized variable: " + name);
    }
    guards_.addPredicateGuard(
        name,
        binding.scopeId,
        binding.value,
        std::move(predicate),
        std::move(description),
        currentScopeId());
}

void Environment::addTypedGuard(
    const std::string& name,
    TypedGuardCallback evaluator,
    std::string description
) {
    const VariableBinding& binding = getBinding(name);
    if (!binding.initialized || !binding.value.isInitialized()) {
        throw std::runtime_error(
            "Cannot attach guard to uninitialized variable: " + name);
    }
    guards_.addTypedGuard(
        name,
        binding.scopeId,
        binding.value,
        std::move(evaluator),
        std::move(description),
        currentScopeId());
}

void Environment::addExpressionGuard(const Expr& condition) {
    const auto vars = GuardManager::collectVariables(condition);
    std::size_t varScopeId = 0;

    // Bind guard target to the innermost referenced variable's scope.
    for (const auto& varName : vars) {
        const VariableBinding& b = getBinding(varName);
        if (!b.initialized || !b.value.isInitialized()) {
            throw std::runtime_error(
                "Guard expression references uninitialized variable: " + varName);
        }
        if (b.scopeId > varScopeId) {
            varScopeId = b.scopeId;
        }
    }

    guards_.addExpressionGuard(
        condition,
        varScopeId,
        makeResolver(),
        currentScopeId());
}

bool Environment::hasGuard(const std::string& name) const {
    const VariableBinding* b = findBinding(name);
    if (!b) {
        return false;
    }
    return guards_.hasGuard(name, b->scopeId);
}

std::vector<PreviewVariableInfo> Environment::exportActiveVariables() const {
    std::vector<PreviewVariableInfo> result;

    for (const auto& frame : frames_) {
        for (const auto& varName : frame.declarationOrder) {
            const auto it = frame.variables.find(varName);
            if (it == frame.variables.end()) {
                continue;
            }
            const VariableBinding& b = it->second;
            Value currentVal = b.value;
            if (b.readThroughHook && b.initialized) {
                currentVal = b.readThroughHook();
            }

            result.push_back(PreviewVariableInfo{
                b.name,
                b.type.toString(),
                b.initialized ? currentVal.toString() : "<uninitialized>",
                b.locked,
                formatScopeLabel(b.scopeId)
            });
        }
    }

    return result;
}

std::vector<VariableSnapshot> Environment::captureVariables(
    const std::vector<std::string>& names
) const {
    std::vector<VariableSnapshot> snapshots;
    snapshots.reserve(names.size());

    for (const auto& name : names) {
        const VariableBinding* binding = findBinding(name);
        if (!binding) {
            throw std::runtime_error(
                "Cannot checkpoint unknown variable: " + name);
        }

        Value capturedValue = binding->value;
        if (binding->readThroughHook && binding->initialized) {
            capturedValue = binding->readThroughHook();
        }
        capturedValue.setBindingId(binding->bindingId);

        // Value has value-copy semantics, so std::any{capturedValue} owns a deep copy.
        snapshots.push_back(VariableSnapshot{
            binding->name,
            std::any{capturedValue},
            binding->type.toString(),
            binding->scopeId,
            binding->initialized
        });
    }

    return snapshots;
}

void Environment::restoreVariablesAtomically(
    const std::vector<VariableSnapshot>& variables
) {
    // Stage a full copy of the environment frames for atomic rollback on failure.
    std::vector<ScopeFrame> candidateFrames = frames_;

    for (const auto& snapshot : variables) {
        VariableBinding* target =
            findBindingInFramesMutable(candidateFrames, snapshot.name);
        if (!target) {
            throw std::runtime_error(
                "Cannot restore unknown variable: " + snapshot.name);
        }
        if (target->scopeId != snapshot.scopeId) {
            throw std::runtime_error(
                "Scope mismatch restoring variable: " + snapshot.name);
        }
        if (target->type.toString() != snapshot.declaredType) {
            throw std::runtime_error(
                "Type mismatch restoring variable: " + snapshot.name);
        }
        if (target->locked) {
            throw std::runtime_error(
                "Cannot restore locked variable: " + snapshot.name);
        }

        const auto* storedValue = std::any_cast<Value>(&snapshot.value);
        if (!storedValue) {
            throw std::runtime_error(
                "Invalid snapshot value payload for variable: " + snapshot.name);
        }

        // Verify binding identity if recorded, rejecting redeclared bindings in the same scope (A38, A41).
        if (storedValue->bindingId() != 0 &&
            target->bindingId != 0 &&
            storedValue->bindingId() != target->bindingId) {
            throw std::runtime_error(
                "Binding identity mismatch restoring redeclared variable: " +
                snapshot.name);
        }

        if (snapshot.initialized &&
            (!storedValue->isInitialized() ||
             storedValue->type() != target->type)) {
            throw std::runtime_error(
                "Snapshot value type does not match declared type for: " +
                snapshot.name);
        }

        target->value = *storedValue;
        target->value.setBindingId(target->bindingId);
        target->initialized = snapshot.initialized;
    }

    // Validate all active guards against the staged candidate state before committing.
    guards_.validateAll(makeResolverForFrames(candidateFrames));

    // Commit atomically and update any bound C++ references (Assumption A39).
    frames_.swap(candidateFrames);
    for (const auto& snapshot : variables) {
        VariableBinding* committed = findBindingMutable(snapshot.name);
        if (committed && committed->writeBackHook && committed->initialized) {
            committed->writeBackHook(committed->value);
        }
    }
}

void registerPreviewEnvironment(const Environment* env) {
    activePreviewEnvironmentStorage() = env;
}

void clearPreviewEnvironment() {
    activePreviewEnvironmentStorage() = nullptr;
}

void previewActiveVariables(std::ostream& out) {
    const Environment* env = activePreviewEnvironmentStorage();
    if (!env) {
        throw std::runtime_error(
            "preview() requires a registered Environment snapshot provider");
    }

    auto variables = env->exportActiveVariables();
    out << "Variables (" << variables.size() << "):\n";
    for (const auto& var : variables) {
        out << "  "
            << var.name
            << " : " << var.type
            << " = " << var.value
            << " [" << (var.locked ? "locked" : "unlocked") << "]";
        if (!var.scope.empty()) {
            out << " scope=" << var.scope;
        }
        out << '\n';
    }
}

} // namespace gash

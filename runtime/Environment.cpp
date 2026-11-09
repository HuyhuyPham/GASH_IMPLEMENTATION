#include "Environment.h"
#include "Preview.h"

#include <algorithm>
#include <stdexcept>
#include <utility>

namespace gash {

namespace {

std::string formatScopeLabel(std::size_t scopeId) {
    if (scopeId == 1) {
        return "global";
    }
    return "scope#" + std::to_string(scopeId);
}

template <typename T>
Type listElementTypeFor();

template <>
Type listElementTypeFor<int>() { return Type::Int(); }

template <>
Type listElementTypeFor<double>() { return Type::Float(); }

template <>
Type listElementTypeFor<bool>() { return Type::Bool(); }

template <>
Type listElementTypeFor<char>() { return Type::Char(); }

template <>
Type listElementTypeFor<std::string>() { return Type::String(); }

template <typename T>
T extractElement(const Value& v);

template <>
int extractElement<int>(const Value& v) { return v.asInt(); }

template <>
double extractElement<double>(const Value& v) { return v.asFloat(); }

template <>
bool extractElement<bool>(const Value& v) { return v.asBool(); }

template <>
char extractElement<char>(const Value& v) { return v.asChar(); }

template <>
std::string extractElement<std::string>(const Value& v) { return v.asString(); }

template <typename T>
Value convertListToValue(const List<T>& cppList) {
    const Type elemType = listElementTypeFor<T>();
    ListValue lv(
        elemType,
        static_cast<std::int64_t>(cppList.size()),
        cppList.capacity());
    for (std::size_t i = 0; i < cppList.size(); ++i) {
        const auto idx = static_cast<std::int64_t>(i);
        if (cppList.isInitialized(idx)) {
            lv.set(idx, Value(cppList.get(idx)));
        }
    }
    return Value(std::move(lv));
}

template <typename T>
void applyValueToList(const Value& v, List<T>& cppList) {
    const ListValue& lv = v.asList();
    cppList.resize(0);
    if (lv.capacity() > 0) {
        cppList.resize(static_cast<std::int64_t>(lv.capacity()));
    }
    cppList.resize(static_cast<std::int64_t>(lv.size()));
    for (std::size_t i = 0; i < lv.size(); ++i) {
        const auto idx = static_cast<std::int64_t>(i);
        if (lv.isInitialized(idx)) {
            cppList.set(idx, extractElement<T>(lv.get(idx)));
        }
    }
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
    liveGuards_.erase(
        std::remove_if(
            liveGuards_.begin(),
            liveGuards_.end(),
            [poppedId](const LiveGuardEntry& entry) {
                return entry.registrationScopeId == poppedId;
            }),
        liveGuards_.end());
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
    b->value = Value::Int(cppVar);
    b->value.setBindingId(b->bindingId);
    b->initialized = true;
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
    b->value = Value::Float(cppVar);
    b->value.setBindingId(b->bindingId);
    b->initialized = true;
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
    b->value = Value::Bool(cppVar);
    b->value.setBindingId(b->bindingId);
    b->initialized = true;
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
    b->value = Value::Char(cppVar);
    b->value.setBindingId(b->bindingId);
    b->initialized = true;
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
    b->value = Value::String(cppVar);
    b->value.setBindingId(b->bindingId);
    b->initialized = true;
    b->writeBackHook = [&cppVar](const Value& v) { cppVar = v.asString(); };
    b->readThroughHook = [&cppVar]() { return Value::String(cppVar); };
}

void Environment::bindReference(const std::string& name, List<int>& cppList) {
    const Type listType = Type::ListOf(Type::Int());
    if (!hasVariableInCurrentScope(name)) {
        define(name, listType, convertListToValue(cppList));
    }
    VariableBinding* b = findBindingMutable(name);
    if (!b || b->type != listType) {
        throw std::runtime_error("Cannot bind 'List<int>&' to variable '" + name + "'");
    }
    b->value = convertListToValue(cppList);
    b->value.setBindingId(b->bindingId);
    b->initialized = true;
    b->writeBackHook = [&cppList](const Value& v) { applyValueToList(v, cppList); };
    b->readThroughHook = [&cppList]() { return convertListToValue(cppList); };
}

void Environment::bindReference(const std::string& name, List<double>& cppList) {
    const Type listType = Type::ListOf(Type::Float());
    if (!hasVariableInCurrentScope(name)) {
        define(name, listType, convertListToValue(cppList));
    }
    VariableBinding* b = findBindingMutable(name);
    if (!b || b->type != listType) {
        throw std::runtime_error("Cannot bind 'List<double>&' to variable '" + name + "'");
    }
    b->value = convertListToValue(cppList);
    b->value.setBindingId(b->bindingId);
    b->initialized = true;
    b->writeBackHook = [&cppList](const Value& v) { applyValueToList(v, cppList); };
    b->readThroughHook = [&cppList]() { return convertListToValue(cppList); };
}

void Environment::bindReference(const std::string& name, List<bool>& cppList) {
    const Type listType = Type::ListOf(Type::Bool());
    if (!hasVariableInCurrentScope(name)) {
        define(name, listType, convertListToValue(cppList));
    }
    VariableBinding* b = findBindingMutable(name);
    if (!b || b->type != listType) {
        throw std::runtime_error("Cannot bind 'List<bool>&' to variable '" + name + "'");
    }
    b->value = convertListToValue(cppList);
    b->value.setBindingId(b->bindingId);
    b->initialized = true;
    b->writeBackHook = [&cppList](const Value& v) { applyValueToList(v, cppList); };
    b->readThroughHook = [&cppList]() { return convertListToValue(cppList); };
}

void Environment::bindReference(const std::string& name, List<char>& cppList) {
    const Type listType = Type::ListOf(Type::Char());
    if (!hasVariableInCurrentScope(name)) {
        define(name, listType, convertListToValue(cppList));
    }
    VariableBinding* b = findBindingMutable(name);
    if (!b || b->type != listType) {
        throw std::runtime_error("Cannot bind 'List<char>&' to variable '" + name + "'");
    }
    b->value = convertListToValue(cppList);
    b->value.setBindingId(b->bindingId);
    b->initialized = true;
    b->writeBackHook = [&cppList](const Value& v) { applyValueToList(v, cppList); };
    b->readThroughHook = [&cppList]() { return convertListToValue(cppList); };
}

void Environment::bindReference(const std::string& name, List<std::string>& cppList) {
    const Type listType = Type::ListOf(Type::String());
    if (!hasVariableInCurrentScope(name)) {
        define(name, listType, convertListToValue(cppList));
    }
    VariableBinding* b = findBindingMutable(name);
    if (!b || b->type != listType) {
        throw std::runtime_error("Cannot bind 'List<std::string>&' to variable '" + name + "'");
    }
    b->value = convertListToValue(cppList);
    b->value.setBindingId(b->bindingId);
    b->initialized = true;
    b->writeBackHook = [&cppList](const Value& v) { applyValueToList(v, cppList); };
    b->readThroughHook = [&cppList]() { return convertListToValue(cppList); };
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
        const VariableBinding& b = this->getBinding(name);
        if (!b.initialized || !b.value.isInitialized()) {
            throw std::runtime_error("Read of uninitialized variable: " + name);
        }
        if (b.readThroughHook) {
            return b.readThroughHook();
        }
        return b.value;
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

void Environment::beginMutation(const std::string& name, bool isListOp) const {
    const VariableBinding* binding = findBinding(name);
    if (!binding) {
        return;
    }
    if (binding->locked) {
        if (isListOp) {
            throw std::runtime_error(
                "Cannot modify element or size of locked list '" + name + "'");
        }
        throw std::runtime_error("Cannot modify locked variable: " + name);
    }
}

Value Environment::snapshotCurrentValue(const std::string& name) const {
    const VariableBinding* binding = findBinding(name);
    if (!binding) {
        return Value();
    }
    if (binding->readThroughHook && binding->initialized) {
        Value v = binding->readThroughHook();
        v.setBindingId(binding->bindingId);
        return v;
    }
    return binding->value;
}

void Environment::commitOrRollbackMutation(
    const std::string& name,
    const Value& /*oldVal*/
) {
    VariableBinding* binding = findBindingMutable(name);
    if (!binding) {
        validateLiveGuards();
        return;
    }
    Value candidateVal = binding->readThroughHook
                             ? binding->readThroughHook()
                             : binding->value;
    candidateVal.setBindingId(binding->bindingId);

    guards_.validateCandidate(
        name,
        binding->scopeId,
        candidateVal,
        makeResolver());
    validateLiveGuards();

    binding->value = std::move(candidateVal);
    binding->initialized = true;
}

void Environment::rollbackValue(const std::string& name, const Value& oldVal) {
    VariableBinding* binding = findBindingMutable(name);
    if (!binding || !oldVal.isInitialized()) {
        return;
    }
    binding->value = oldVal;
    binding->value.setBindingId(binding->bindingId);
    if (binding->writeBackHook) {
        binding->writeBackHook(binding->value);
    }
}

void Environment::validateLiveGuards() const {
    for (const auto& lg : liveGuards_) {
        if (!lg.check()) {
            if (!lg.description.empty()) {
                throw std::runtime_error("Guard violation: " + lg.description);
            }
            throw std::runtime_error("Guard violation");
        }
    }
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

    // Validate candidate assignment against AST/callback guards before mutating.
    guards_.validateCandidate(
        name,
        binding->scopeId,
        newValue,
        makeResolver());

    Value previousValue = binding->readThroughHook
                              ? binding->readThroughHook()
                              : binding->value;
    previousValue.setBindingId(binding->bindingId);

    binding->value = newValue;
    binding->value.setBindingId(binding->bindingId);
    binding->initialized = true;
    if (binding->writeBackHook) {
        binding->writeBackHook(binding->value);
    }

    try {
        validateLiveGuards();
    } catch (...) {
        binding->value = previousValue;
        if (binding->writeBackHook && previousValue.isInitialized()) {
            binding->writeBackHook(previousValue);
        }
        throw;
    }
}

Value Environment::getListElement(
    const std::string& name,
    std::int64_t index
) const {
    const VariableBinding& binding = getBinding(name);
    if (!binding.initialized || !binding.value.isInitialized()) {
        throw std::runtime_error("Read of uninitialized variable: " + name);
    }
    Value listVal = binding.readThroughHook ? binding.readThroughHook() : binding.value;
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

    Value currentVal = binding->readThroughHook ? binding->readThroughHook() : binding->value;
    currentVal.setBindingId(binding->bindingId);

    // Stage a deep copy of the list and apply the element change on the candidate.
    ListValue candidateList = currentVal.asList();
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
    if (binding->writeBackHook) {
        binding->writeBackHook(binding->value);
    }

    try {
        validateLiveGuards();
    } catch (...) {
        binding->value = currentVal;
        if (binding->writeBackHook) {
            binding->writeBackHook(currentVal);
        }
        throw;
    }
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

    Value currentVal = binding->readThroughHook ? binding->readThroughHook() : binding->value;
    currentVal.setBindingId(binding->bindingId);

    ListValue candidateList = currentVal.asList();
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
    if (binding->writeBackHook) {
        binding->writeBackHook(binding->value);
    }

    try {
        validateLiveGuards();
    } catch (...) {
        binding->value = currentVal;
        if (binding->writeBackHook) {
            binding->writeBackHook(currentVal);
        }
        throw;
    }
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
    Value currentVal = binding.readThroughHook ? binding.readThroughHook() : binding.value;
    guards_.addPredicateGuard(
        name,
        binding.scopeId,
        currentVal,
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
    Value currentVal = binding.readThroughHook ? binding.readThroughHook() : binding.value;
    guards_.addTypedGuard(
        name,
        binding.scopeId,
        currentVal,
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

void Environment::addLiveGuard(
    std::function<bool()> check,
    std::string description
) {
    if (!check) {
        throw std::invalid_argument("Live guard callback cannot be empty");
    }
    if (!check()) {
        if (!description.empty()) {
            throw std::runtime_error(
                "Guard violation on initial value: " + description);
        }
        throw std::runtime_error("Guard violation");
    }
    liveGuards_.push_back(LiveGuardEntry{
        currentScopeId(),
        std::move(check),
        std::move(description)
    });
}

bool Environment::hasGuard(const std::string& name) const {
    const VariableBinding* b = findBinding(name);
    if (!b) {
        return false;
    }
    return guards_.hasGuard(name, b->scopeId) || !liveGuards_.empty();
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
    // Sync current live C++ values into frames_ before staging so rollback is exact.
    for (auto& frame : frames_) {
        for (auto& kv : frame.variables) {
            VariableBinding& b = kv.second;
            if (b.readThroughHook && b.initialized) {
                b.value = b.readThroughHook();
                b.value.setBindingId(b.bindingId);
            }
        }
    }

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

    // Validate all AST/callback guards against the staged candidate state before committing.
    guards_.validateAll(makeResolverForFrames(candidateFrames));

    // Commit candidateFrames and apply writeBackHook, then validate live C++ guards.
    frames_.swap(candidateFrames);
    for (const auto& snapshot : variables) {
        VariableBinding* committed = findBindingMutable(snapshot.name);
        if (committed && committed->writeBackHook && committed->initialized) {
            committed->writeBackHook(committed->value);
        }
    }

    try {
        validateLiveGuards();
    } catch (...) {
        // Roll back frames_ and bound C++ variables atomically.
        frames_.swap(candidateFrames);
        for (const auto& snapshot : variables) {
            VariableBinding* previous = findBindingMutable(snapshot.name);
            if (previous && previous->writeBackHook && previous->initialized) {
                previous->writeBackHook(previous->value);
            }
        }
        throw;
    }
}

void registerPreviewEnvironment(const Environment* env) {
    if (!env) {
        setPreviewProvider({});
        return;
    }
    setPreviewProvider(env->createPreviewProvider<PreviewVariableSnapshot>());
}

void clearPreviewEnvironment() {
    setPreviewProvider({});
}

void previewActiveVariables(std::ostream& out) {
    try {
        preview(out);
    } catch (const std::logic_error& err) {
        throw std::runtime_error(err.what());
    }
}

} // namespace gash

# GASH Compiler and Runtime Architecture

**Course:** COMP2001 — Language Models, Translation and Execution

---

## 1. Pipeline Overview

The GASH implementation processes programs through four coordinated stages:

1. **Lexer (`include/Lexer.h`, `src/Lexer.cpp`)**: Tokenizes raw `.gash` source code into a stream of `Token` objects (`include/Token.h`), reporting lexical errors with line and column coordinates (`include/LexicalError.h`).
2. **Parser & AST (`include/Parser.h`, `src/Parser.cpp`, `include/AST.h`)**: Performs recursive-descent syntax analysis to construct an Abstract Syntax Tree (`Program`, `Stmt`, `Expr`).
3. **Semantic Analyzer & Type System (`include/Type.h`, `include/Semantic.h`, `src/Semantic.cpp`)**: Validates lexical scopes, symbol declarations, strict type rules, explicit conversions, function signatures, recursive return type inference, loop/function statement contexts, and static transparency rules.
4. **Code Generator & Runtime (`include/CodeGenerator.h`, `src/CodeGenerator.cpp`, `runtime/*`)**: Emits C++17 code backed by runtime support for `Environment`, `Value`, `GuardManager`, `CheckpointManager`, `List<T>`, `BudgetScope`, `inspect()`, and `preview()`.

---

## 2. Type System, Symbols, and Scopes (`include/Type.h`)

- **Primitive Types**: `int`, `float`, `bool`, `char`, `string`, plus `void` for non-returning functions.
- **Parameterized List Types**: `list<T>` where `T` is any primitive type (`list<int>`, `list<float>`, `list<bool>`, `list<char>`, `list<string>`). Nested lists are rejected.
- **C++17 Type Mapping (Assumption A27)**:
  - `int` -> `int`
  - `float` -> `double`
  - `bool` -> `bool`
  - `char` -> `char`
  - `string` -> `std::string`
  - `list<T>` -> `gash::List<T>`
- **Lexical Scope Stack (`ScopeStack`)**: Tracks nested scopes with unique monotonic `scopeId` values. Rejects duplicate declarations within the same scope while permitting lexical shadowing in inner blocks.

---

## 3. Semantic Analysis (`include/Semantic.h`, `src/Semantic.cpp`)

- **Strict Typing**: Binary operators require identical operand types (`int + int`, `float + float`, `string + string`). Implicit `int` to `float` coercion is prohibited; explicit conversions `int(x)`, `float(x)`, and `string(x)` must be used (Assumptions A31, A40).
- **Functions and Return Inference (Assumption A29)**:
  - Parameter types are explicitly declared and validated.
  - Return types are inferred from `return` statements in the function body, including pre-inference for recursive functions.
  - All `return` paths in a function must return the exact same type. Functions with no `return` statement have return type `void`.
- **Control Flow & Statement Contexts (Assumption A32)**:
  - `if` and `while` conditions must have type `bool`.
  - `repeat` and `budget` limits must have type `int`.
  - `exit` and `skip` are rejected outside of `while` or `repeat` loops.
  - `return` is rejected outside of `func` declarations.
- **Transparency Statements (Assumption A34)**:
  - `guard expr;` requires `expr` to have type `bool` and reference declared variables.
  - `lock x;` and `unlock x;` require `x` to be a declared variable in scope.
  - `checkpoint label(v1, ...);` enforces unique checkpoint labels, non-empty variable lists, no duplicate variable names, and valid variable bindings.
  - `restore(label);` verifies that `label` exists and that all captured variables remain in scope with matching types and scope identities.

---

## 4. Runtime Environment, Locks, and Guards (`runtime/`)

### 4.1 Runtime Values (`runtime/Value.h`)
- `Value` and `ListValue` use deep-copy value semantics.
- Provides runtime conversion helpers (`gash::toInt`, `gash::toFloat`, `gash::toString`) and `gash::input()` used by generated C++17 code.

### 4.2 Environment & Central Assignment API (`runtime/Environment.h`, `runtime/Environment.cpp`)
- Maintains a stack of `ScopeFrame` records, assigning each scope a unique `scopeId` and each variable declaration a unique `bindingId` (Assumptions A38, A41).
- Provides `ScopeGuard` for automatic RAII scope and guard cleanup on normal block exit or exception unwinding (Assumption A53).
- Central mutation methods (`assign`, `assignListElement`, `extendList`) enforce type matching, lock permissions, and guard invariants before committing changes (Assumptions A13, A14, A30, A33, A46).
- Supports two-way binding with generated C++ variables via `bindReference()` (Assumption A39) and integrates with `gash::preview()` via `registerPreviewEnvironment()` (Assumptions A15, A48, A49).

### 4.3 Lock and Unlock Semantics
- `lock(name)` prevents any mutation to `name` (`assign`, `assignListElement`, `extendList`, and checkpoint `restore`), while still allowing read access.
- `unlock(name)` restores write permission.
- Repeated `lock` on an already-locked variable and repeated `unlock` on an already-unlocked variable are safe no-ops.

### 4.4 Mutation Guards (`runtime/Guard.h`, `runtime/Guard.cpp`)
- Supports both predicate/typed callbacks and AST expression guards (`GuardStmt`).
- **Pre-registration Validation**: Verifies that the guard evaluates to `bool` and that the current initial variable value(s) satisfy the guard condition.
- **Pre-commit Candidate Validation**: Evaluates all active guards against candidate values before mutating live state. Any guard violation throws `std::runtime_error` and leaves all live variables unchanged.
- **Atomic Checkpoint Restoration (`runtime/Checkpoint.h`, `runtime/Checkpoint.cpp`)**: Validates all target variables, types, `scopeId`s, `bindingId`s, lock states, and guards on a staged copy of the environment before committing atomically (Assumptions A18, A35–A38, A43).

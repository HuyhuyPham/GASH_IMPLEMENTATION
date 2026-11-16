# GASH Compiler and Runtime Architecture

**Course:** COMP2001 — Language Models, Translation and Execution

**Implementation:** C++17 Source-to-Source Compiler

---

## 1. Architecture Overview

GASH is a statically typed programming language implemented as a source-to-source compiler. It translates GASH programs into C++17 code, which is then compiled into an executable using a C++ compiler.

The implementation separates lexical analysis, parsing, semantic analysis, code generation, and runtime support into coordinated components.

### 1.1 Compilation Pipeline

```text
GASH Source Code (.gash)
         |
         v
       Lexer
         |
         v
    Token Stream
         |
         v
  Recursive-Descent Parser
         |
         v
 Abstract Syntax Tree (AST)
         |
         v
   Semantic Analyzer
         |
         v
     Code Generator
         |
         v
  Generated C++17 Source
         |
         v
     C++ Compiler (g++)
         |
         v
    Native Executable
         |
         v
 GASH Runtime Components
```

### 1.2 Main Components

| Component         | Implementation                                        | Responsibility                                                                      |
| ----------------- | ----------------------------------------------------- | ----------------------------------------------------------------------------------- |
| Lexer             | `include/Lexer.h`, `src/Lexer.cpp`                    | Converts source code into tokens and identifies lexical errors                      |
| Parser and AST    | `include/Parser.h`, `src/Parser.cpp`, `include/AST.h` | Validates grammatical structure and constructs the AST                              |
| Semantic Analyzer | `include/Semantic.h`, `src/Semantic.cpp`              | Performs type checking, scope resolution, and semantic validation                   |
| Code Generator    | `include/CodeGenerator.h`, `src/CodeGenerator.cpp`    | Translates validated AST constructs into C++17                                      |
| Compiler Pipeline | `src/CompilerPipeline.cpp`                            | Coordinates compilation and execution of generated code                             |
| Runtime           | `runtime/`                                            | Supports variable state, lists, guards, locks, checkpoints, budgets, and inspection |

The command-line interface provides modes for displaying tokens, checking programs, generating C++ source code, and executing GASH programs.

---

## 2. Type System and Scope Management

### 2.1 Type System

GASH supports five primitive types:

- `int`
- `float`
- `bool`
- `char`
- `string`

The compiler also represents `void` as the return type of functions without a value-returning statement.

Lists are parameterized by a primitive element type, such as `list<int>` or `list<string>`. Nested lists are not supported.

### 2.2 C++17 Type Mapping

| GASH type | Generated C++17 type |
| --------- | -------------------- |
| `int`     | `int`                |
| `float`   | `double`             |
| `bool`    | `bool`               |
| `char`    | `char`               |
| `string`  | `std::string`        |
| `list<T>` | `gash::List<T>`      |

This mapping allows generated code to use native C++ types while relying on GASH runtime components for language-specific behaviour.

### 2.3 Lexical Scopes

The semantic analyzer uses a scope stack to track variable declarations and resolve identifiers.

The runtime Environment maintains corresponding scope frames. Each frame has a unique `scopeId`, and each variable declaration receives a unique `bindingId`.

This distinguishes different variable declarations, including variables that have the same name in different scopes.

Variables in inner scopes can shadow variables from outer scopes. Duplicate declarations within the same scope are rejected.

---

## 3. Semantic Analysis

The semantic analyzer operates on the AST and validates programs before C++ generation.

### 3.1 Static Type Checking

GASH enforces strict type compatibility.

Arithmetic and comparison operations are checked against the supported operand types. Implicit numeric conversions between `int` and `float` are prohibited.

Explicit conversion functions include:

```gash
var x = 10;
var y = float(x);
var z = string(x);
```

Conversions are checked during semantic analysis and implemented through generated runtime conversion helpers.

### 3.2 Functions and Return Types

Function parameters require explicit types.

Function return types are inferred from value-returning statements. Multiple value-returning statements must use compatible types, and functions without a return statement are represented as returning `void`.

The semantic analyzer also supports return-type inference for recursive functions.

### 3.3 Control Flow Validation

The semantic analyzer enforces the following requirements:

- `if` and `while` conditions must have type `bool`.
- `repeat` counts and `budget` limits must have type `int`.
- `exit` and `skip` may only appear within loops.
- `return` may only appear within functions.
- Assignments must respect declared types and valid assignment targets.

### 3.4 Transparency Statement Validation

The semantic analyzer also checks language-specific transparency constructs:

- `guard` conditions must have Boolean type.
- `lock` and `unlock` must reference declared variables.
- Checkpoints must contain valid variable references and comply with label and scope restrictions.
- Restoration must reference a valid checkpoint and satisfy applicable semantic constraints.

Runtime checks provide additional validation when programs execute.

---

## 4. Runtime Values and Lists

### 4.1 Runtime Value Representation

**Implementation:** `runtime/Value.h`

The runtime uses the `Value` class to represent dynamically stored GASH values.

Its underlying storage is a `std::variant` containing supported primitive types, `ListValue`, and an uninitialized state.

The class provides:

- Runtime type identification
- Typed access methods
- Initialization checks
- Explicit conversion methods
- Value comparison
- String representations for diagnostics and inspection
- Binding identity metadata for checkpoint restoration

### 4.2 List Representation

**Implementation:** `runtime/List.h`, `runtime/Value.h`

GASH lists support primitive element types and maintain information about their size, capacity, and initialized elements.

List operations include element access, element assignment, and resizing.

The runtime validates element types and indexes, rejecting invalid access or incompatible assignments.

Both `List<T>` and `ListValue` use value-copy semantics. Copying a list produces independent element storage rather than sharing mutable list data.

The use of C++ containers and RAII supports automatic resource cleanup.

---

## 5. Runtime Environment

**Implementation:** `runtime/Environment.h`, `runtime/Environment.cpp`

The `Environment` class manages runtime variable state, lexical scopes, mutation permissions, guards, and checkpoint integration.

### 5.1 Variable Bindings

Each variable is represented by a `VariableBinding` containing:

- Variable name and declared type
- Current value and initialization state
- Scope and binding identifiers
- Lock state
- Optional read-through and write-back callbacks

These callbacks allow runtime metadata to remain connected to variables declared in generated C++.

### 5.2 Environment and Generated C++ Integration

The `bindReference()` methods connect supported C++ primitive and list variables to corresponding Environment bindings.

Read-through callbacks retrieve the current values of generated C++ variables, while write-back callbacks propagate restored or rolled-back values into those variables.

This integration is important because the generated program stores native C++ variables while the Environment manages additional GASH runtime state.

### 5.3 Central Mutation Interface

The Environment provides direct operations such as:

- `assign()` — Assigns a runtime value to a variable.
- `assignListElement()` — Updates an indexed list element.
- `extendList()` — Resizes a list.

The code generator also uses template-based mutation wrappers:

- `mutateVar()` — Handles generated variable assignments.
- `mutateList()` — Handles generated list mutations that return a value.
- `mutateListVoid()` — Handles list mutations without a return value.

These wrappers check mutation permissions, capture previous values, perform the requested operation, and validate applicable guards.

If validation fails, the runtime attempts to restore the previous value before propagating the error.

### 5.4 Scope Management and Cleanup

The Environment uses `pushScope()` and `popScope()` to manage runtime scope frames.

`ScopeGuard` provides RAII-based scope management. It enters a scope when constructed and removes that scope when destroyed.

When a scope ends, the Environment removes guards registered within that scope and discards its variable bindings.

This supports cleanup during both normal execution and exception unwinding.

---

## 6. Lock and Unlock

Locking is implemented as part of each variable's runtime binding.

```gash
var x = 10;

lock x;
unlock x;

x = 20;
```

Calling `lock()` marks a variable as locked, while `unlock()` restores its write permission.

A locked variable remains readable but cannot be modified through the Environment's controlled mutation operations.

For lists, the lock also restricts element modification and resizing.

Checkpoint restoration checks whether the target variables are locked before applying saved values.

Repeated locking of an already-locked variable and repeated unlocking of an already-unlocked variable are safe no-ops.

Attempts to lock or unlock an undeclared variable produce runtime errors.

---

## 7. Mutation Guards

**Implementation:** `runtime/Guard.h`, `runtime/Guard.cpp`

Guards allow programmers to define conditions that restrict changes to program state.

For example:

```gash
var balance = 100;

guard balance >= 0;
```

### 7.1 Guard Representation

The runtime uses `GuardManager` to manage active guard rules.

A `GuardRule` stores the information needed to evaluate a condition, including its associated variables, scope metadata, description, and evaluation callback.

Supported guard mechanisms include predicate callbacks, typed callbacks, AST expression guards, and live callbacks registered through the Environment.

### 7.2 Guard Registration

When a guard is registered, the runtime evaluates its condition against the current state.

A guard must evaluate to a Boolean result, and its initial condition must be satisfied.

Invalid initial conditions are rejected instead of being registered.

### 7.3 Mutation Validation

The `GuardManager` provides two important operations:

- `validateCandidate()` — Validates a proposed update against applicable registered guard rules.
- `validateAll()` — Validates active guard rules against a complete candidate state.

The generated C++ program registers live guard callbacks using `Environment::addLiveGuard()`.

These callbacks are checked after a controlled mutation. If a guard is violated, the Environment restores the previous value through its rollback mechanism and reports an error.

### 7.4 Guard Lifetime

Guards are associated with lexical scope information.

When a scope ends, the Environment removes the relevant guard registrations. This prevents guards declared within completed scopes from remaining active.

---

## 8. Checkpoint and Restore

**Implementation:** `runtime/Checkpoint.h`, `runtime/Checkpoint.cpp`, `runtime/Environment.cpp`

Checkpoint and restore operations allow GASH programs to save selected variable states and restore them later.

```gash
var x = 10;

checkpoint initial(x);

x = 100;

restore(initial);

output(x);
```

**Expected output:**

```text
10
```

### 8.1 CheckpointManager

`CheckpointManager` stores snapshots associated with unique labels.

The `save()` method validates the checkpoint label and selected variable names before requesting snapshots from the Environment.

It rejects:

- Empty checkpoint labels
- Duplicate checkpoint labels
- Empty variable lists
- Duplicate variable names
- Incomplete or inconsistent snapshot results

A checkpoint is stored only after successful capture and validation.

### 8.2 Variable Snapshots

Each `VariableSnapshot` contains:

- Variable name
- Saved value
- Declared type
- Scope identifier
- Initialization state

The saved `Value` also carries binding identity metadata.

Because runtime values use value-copy semantics, snapshots preserve independent copies of variable values, including list data.

### 8.3 Runtime Integration

`CheckpointEnvironment` defines the interface between the checkpoint manager and the execution environment.

Its main operations are:

- `captureVariables()` — Captures selected variable states.
- `restoreVariablesAtomically()` — Validates and restores saved states.

The concrete `Environment` implements this interface.

This separation keeps checkpoint storage and restoration validation in distinct components.

### 8.4 Restoration Process

Checkpoint restoration follows a staged process:

1. Synchronize current generated C++ values with runtime bindings.
2. Copy the current scope frames into a candidate state.
3. Validate target variable existence, type, scope identity, binding identity, and lock state.
4. Apply saved values to the candidate state.
5. Validate registered guards against the candidate state.
6. Commit the candidate frames and propagate restored values to generated C++ variables.
7. Check live guards and roll back if their validation fails.

The staged approach prevents ordinary validation failures from partially applying a checkpoint.

The implementation includes rollback for live-guard failures. As with other exception-handling mechanisms, complete exception safety depends on the behaviour of the underlying write-back operations.

---

## 9. Execution Budgets

**Implementation:** `runtime/Budget.h`, `runtime/Budget.cpp`

GASH supports execution budgets that restrict the number of counted operations performed within a block.

```gash
budget (10) {
    output("Hello");
}
```

The generated C++ uses `BudgetScope` to manage budget lifetimes.

The runtime tracks active budgets and decrements their remaining allowances when an instrumented operation is executed.

Nested budgets are supported, with each active budget accounting for operations executed within its scope.

If a budget is exhausted, the runtime raises a `BudgetExceeded` exception.

Budget lifetime management uses RAII, allowing the active budget to be removed when its scope ends.

---

## 10. Inspect and Preview

**Implementation:** `runtime/Inspect.h`, `runtime/Inspect.cpp`, `runtime/Preview.h`, `runtime/Preview.cpp`

GASH provides two mechanisms for observing runtime state.

### 10.1 Inspect

`inspect()` is designed for list inspection.

It provides information about list structure, including its size, capacity, and initialized positions.

Inspection is read-only and does not modify the inspected list.

### 10.2 Preview

`preview()` displays information about active runtime variables.

The Environment exports records containing:

- Variable name
- Declared type
- Current value
- Lock status
- Scope information

The generated program registers the active Environment through `registerPreviewEnvironment()`.

The preview component uses this registered provider to obtain current variable information.

This separation allows preview formatting to remain independent of the Environment's internal storage structure.

---

## 11. Compilation and Execution

The command-line interface exposes several development and execution modes.

```bash
./build/gash --tokens examples/program.gash
./build/gash --check examples/program.gash
./build/gash --emit-cpp examples/program.gash
./build/gash --run examples/program.gash
```

These modes allow users to inspect tokenization, validate syntax and semantics, generate C++ source, and execute compiled programs.

Generated C++ includes the runtime support required by GASH-specific constructs.

The runtime components enforce language-specific behaviour that cannot be represented solely by ordinary C++ declarations and operators.

### Build and Test

From the repository root:

```bash
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

The latest reported macOS build passed all **22 registered CTest tests**, including three checkpoint end-to-end tests.

The test suite covers lexer, parser, semantic, code-generation, runtime, and integration functionality.

---

## 12. Design Summary

GASH combines a conventional compiler pipeline with runtime mechanisms that expose and constrain program state.

Its architecture emphasizes four design principles:

1. **Separation of concerns:** Lexical, syntactic, semantic, code-generation, and runtime responsibilities are handled by dedicated components.
2. **Static correctness checks:** Type and scope errors are detected before generated C++ code is executed.
3. **Controlled mutation:** Runtime bindings coordinate locks, guards, and state modifications.
4. **Runtime transparency:** Checkpoints, execution budgets, inspection, and preview provide explicit mechanisms for observing and controlling program behaviour.

The compiler retains familiar imperative syntax while integrating these transparency features into the language itself.

For additional details, see [Grammar.md](Grammar.md), [LanguageReference.md](LanguageReference.md), and the repository [README](../README.md).

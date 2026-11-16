# GASH Compiler and Runtime Architecture

**Course:** COMP2001 — Programming Languages
**Implementation:** C++17 Source-to-Source Compiler

---

## 1. Architecture Overview

Transparent GASH is a statically typed programming language implemented as a source-to-source compiler. It translates `.gash` programs into C++17, which is then compiled into a native executable using `g++`.

The implementation separates lexical analysis, parsing, semantic analysis, code generation, and runtime support into distinct components.

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
 GASH Runtime Execution
          |
          v
    Program Output
```

Each stage has a distinct responsibility:

1. **Lexical analysis:** Recognizes tokens, literals, keywords, identifiers, and operators.
2. **Parsing:** Validates grammatical structure and constructs an Abstract Syntax Tree.
3. **Semantic analysis:** Checks types, declarations, scopes, function calls, and other language constraints.
4. **Code generation:** Translates validated AST nodes into C++17 statements and expressions.
5. **Compilation:** Invokes the host C++ compiler to produce an executable.
6. **Runtime execution:** Executes the generated program with GASH-specific state-management and safety mechanisms.

### 1.2 Main Components

| Component | Implementation | Responsibility |
|---|---|---|
| Lexer | `include/Lexer.h`, `src/Lexer.cpp` | Tokenization and lexical diagnostics |
| Parser and AST | `include/Parser.h`, `src/Parser.cpp`, `include/AST.h` | Syntax validation and AST construction |
| Semantic Analyzer | `include/Semantic.h`, `src/Semantic.cpp` | Static type checking, scopes, and function validation |
| Code Generator | `include/CodeGenerator.h`, `src/CodeGenerator.cpp` | C++17 source generation |
| Compiler Pipeline | `include/CompilerPipeline.h`, `src/CompilerPipeline.cpp` | C++ compilation and process execution |
| Runtime | `runtime/` | Values, lists, Environment, guards, locks, checkpoints, budgets, and inspection |
| CLI | `src/main.cpp` | User-facing compiler commands |

The compiler exposes commands for displaying tokens, checking programs, emitting C++ source, and compiling and executing GASH programs.

---

## 2. Type System and Scope Management

### 2.1 Static Type System

GASH supports five primitive types:

- `int`
- `float`
- `bool`
- `char`
- `string`

Lists are parameterized by a primitive element type, such as `list<int>` or `list<string>`. Nested lists are not supported.

The compiler also represents `void` as the return type of functions without a value-returning statement.

Variable declarations require an initializer, and their types are inferred from the initialization expression.

Types remain fixed after declaration. Incompatible assignments are rejected during semantic analysis.

### 2.2 C++17 Type Mapping

| GASH type | Generated C++ type |
|---|---|
| `int` | `int` |
| `float` | `double` |
| `bool` | `bool` |
| `char` | `char` |
| `string` | `std::string` |
| `list<T>` | `gash::List<T>` |

Generated programs use native C++ types where practical and invoke the GASH runtime to implement language-specific behaviour.

For example, ordinary variables use C++ storage, while the runtime Environment tracks their associated metadata, locks, guards, and checkpoint state.

### 2.3 Lexical Scope Management

The semantic analyzer uses a scope stack to track declarations and resolve identifiers.

Inner scopes may access visible declarations from outer scopes. Variables may shadow other variables declared in enclosing scopes, but duplicate declarations in the same scope are rejected.

The runtime Environment maintains corresponding scope frames.

Each scope receives a unique `scopeId`, and variable bindings receive unique `bindingId` values. These identifiers distinguish declarations with the same name in different scopes.

---

## 3. Semantic Analysis

**Implementation:** `include/Semantic.h`, `src/Semantic.cpp`

The semantic analyzer examines the AST before code generation.

### 3.1 Type Checking

GASH uses strict type compatibility.

For example:

```gash
var x = 10;
var y = float(x);
var text = string(x);
```

Explicit conversion functions permit supported conversions between types.

Semantic analysis checks arithmetic operands, comparisons, assignments, function arguments, return statements, and other expressions.

### 3.2 Function Signatures and Return Types

Function parameters require explicit type annotations.

The parser supports primitive parameter types and typed-list parameters, such as:

```gash
func first(list<int> values) {
    return values[0];
}
```

Function return types are inferred from value-returning statements.

The semantic analyzer checks that inferred return types are consistent and that function calls use compatible arguments.

Recursive functions are supported.

The compiler also maintains return-type information associated with individual `FunctionDeclStmt` AST nodes. This is especially important when nested functions have their own signatures.

### 3.3 Control Flow Validation

Semantic analysis enforces requirements including:

- `if` and `while` conditions must be Boolean.
- `repeat` counts and `budget` limits must be integers.
- `exit` and `skip` must occur in valid loop contexts.
- `return` must occur within functions.
- Assignment targets must be valid.
- Assigned values must satisfy declared types.

Certain constraints are enforced at runtime because their validity depends on values calculated during execution.

For example, a negative `repeat` count is rejected at runtime.

### 3.4 Transparency Statement Validation

Semantic analysis also checks GASH-specific statements:

- Guard conditions must be Boolean.
- Lock and unlock operations must reference declared variables.
- Checkpoints must use valid variable references and labels.
- Restoration must reference a valid checkpoint.
- Scope and type restrictions must be respected.

Additional runtime validation applies during mutations and restoration.

---

## 4. Code Generation

**Implementation:** `include/CodeGenerator.h`, `src/CodeGenerator.cpp`

The code generator translates validated AST constructs into C++17.

Generated code uses C++ declarations, expressions, functions, and control-flow statements, supplemented by runtime helper calls.

### 4.1 Source Generation Strategy

The generated C++ source includes the standard and GASH runtime headers required for its operations.

It establishes shared runtime components, including:

```cpp
static gash::Environment gash_env;
static gash::CheckpointManager gash_checkpoints;
```

These objects coordinate runtime variable metadata, guards, locks, and checkpoint operations.

Top-level function definitions and generated program statements are emitted into the appropriate C++ scopes.

The generated `main()` function initializes runtime services and executes GASH top-level statements.

### 4.2 Global Variable Hoisting

GASH permits functions to access visible global variables.

However, ordinary C++ functions cannot access variables declared locally inside `main()`.

To preserve GASH semantics, the code generator identifies top-level variable declarations and emits their storage at C++ file scope before emitting top-level functions.

Conceptually:

```cpp
static int gash_count{};
```

The corresponding GASH initialization expression is then evaluated at its original position during top-level execution.

The generated program also registers the variable with the runtime Environment:

```cpp
gash_env.bindReference("count", gash_count);
```

This separation between **storage declaration** and **runtime initialization** allows functions to reference global variables while retaining top-level initialization order.

The approach also supports typed global lists through their generated C++ storage types.

### 4.3 Nested Function Generation

GASH permits functions declared inside blocks or other functions.

Ordinary C++17 functions cannot be defined directly inside another function body. The generator therefore represents nested functions using `std::function` and lambda expressions.

Conceptually:

```cpp
std::function<int(int)> gash_helper;

gash_helper = [&](int gash_x) -> int {
    return gash_x + 1;
};
```

This mechanism allows a nested function to capture accessible variables from its enclosing C++ scope.

Declaring the `std::function` variable before assigning the lambda also enables supported recursive calls.

Function return types are obtained from semantic analysis so that the generated callable has an explicit C++ signature.

Nested functions continue to use GASH runtime scope management and parameter bindings.

### 4.4 Expressions and Checked Arithmetic

Ordinary expressions are translated into corresponding C++ expressions when their semantics are compatible.

Operations requiring additional validation use runtime helpers.

For example:

| GASH operation | Generated implementation |
|---|---|
| Division | `gash::safeDivide(...)` |
| Modulo | `gash::safeModulo(...)` |
| Exponentiation | `gash::safePower(...)` |

These helpers provide runtime checks for defined invalid conditions such as division by zero, modulo by zero, and non-finite exponentiation results.

The code generator also checks that a `repeat` count is not negative before executing the loop.

**Implementation boundary:** The current `safePower()` implementation checks whether its floating-point result is finite, but this alone does not guarantee that the result fits into an integer destination type. Integer overflow-range handling should be reviewed separately.

### 4.5 Character and String Escaping

GASH literals must be emitted as valid C++17 literals.

The generator provides distinct escaping logic for strings and characters.

In particular, character escaping handles characters such as the single quote and NUL character so that valid GASH source does not produce malformed C++ literals.

### 4.6 Output Generation

GASH output uses runtime formatting helpers rather than relying solely on C++ stream operators.

Generated calls use:

```cpp
gash::writeOutput(std::cout, value);
```

The formatting layer ensures that:

- Boolean values use `true` and `false`.
- Primitive values are displayed using the supported GASH representation.
- Typed lists can be displayed using their structured list formatting.

This provides more consistent output behaviour across language types.

### 4.7 Generated Function Return Safety

Functions returning a value must not reach the end of their bodies without returning.

For non-void generated functions, the code generator adds a runtime error at the end of the function body.

Conceptually:

```cpp
throw std::runtime_error(
    "Function reached end without returning a value"
);
```

This prevents execution from silently falling through a non-void function, which would otherwise introduce undefined behaviour in C++.

### 4.8 Generated Runtime Exception Handling

The generated `main()` function uses exception handling around program execution.

Runtime exceptions are reported using messages such as:

```text
Runtime error: Division by zero
```

The handler flushes standard output, writes the diagnostic to standard error, and returns a nonzero exit status.

This makes failures more predictable and preserves output produced before a runtime error.

Execution budgets also use a dedicated `BudgetExceeded` exception type, which may be handled by an enclosing budget scope.

---

## 5. Runtime Values and Typed Lists

### 5.1 Runtime Value Representation

**Implementation:** `runtime/Value.h`, `runtime/Value.cpp`

The runtime `Value` abstraction represents dynamically stored GASH values.

It uses `std::variant` to hold supported primitive values, `ListValue`, or an uninitialized state.

The abstraction provides:

- Runtime type identification
- Typed value access
- Initialization checks
- Conversion helpers
- Value comparison
- String representations
- Metadata supporting checkpoint restoration

The runtime separates some declarations and definitions between `Value.h` and `Value.cpp`.

### 5.2 Typed List Representation

**Implementation:** `runtime/List.h`, `runtime/List.cpp`

Typed lists are represented using `gash::List<T>`.

Lists track logical size, capacity, and which positions contain initialized values.

Internally, the implementation uses C++ container facilities including optional element storage, allowing it to distinguish an initialized position from an uninitialized one.

Supported operations include:

- Indexed reads and writes
- Bounds checking
- Element initialization tracking
- List resizing
- Structured inspection
- Value copying

Both typed lists and runtime list values follow value-copy semantics. Copying a list creates independent element storage instead of sharing mutable elements.

### 5.3 Automatic Resource Management

GASH relies on C++ RAII for resource cleanup.

List storage is released when the corresponding C++ objects are destroyed.

This includes ordinary scope exit and applicable exception unwinding.

No explicit GASH `cleanup()` operation is required for list-owned storage.

---

## 6. Runtime Environment

**Implementation:** `runtime/Environment.h`, `runtime/Environment.cpp`

The `Environment` coordinates variable state, scope information, mutation permissions, guard validation, and checkpoint restoration.

### 6.1 Variable Bindings

Each `VariableBinding` records relevant information such as:

- Variable name and type
- Current runtime value
- Initialization state
- Scope and binding identifiers
- Lock state
- Optional read-through and write-back callbacks

This information permits the runtime to identify which variable is being modified and which restrictions apply.

### 6.2 Connecting Generated C++ and Runtime State

Generated programs store many variables in ordinary C++ objects.

The Environment provides `bindReference()` methods to associate these objects with runtime bindings.

Read-through callbacks retrieve values from generated C++ variables.

Write-back callbacks propagate restored or rolled-back runtime values into the generated variables.

This connection is necessary because GASH stores execution data in native C++ objects while tracking additional metadata separately.

### 6.3 Controlled Mutation

The Environment provides operations such as:

- `assign()` — Assign a runtime value.
- `assignListElement()` — Modify a list element.
- `extendList()` — Change a list's size.
- `mutateVar()` — Wrap generated variable assignments.
- `mutateList()` — Wrap generated list mutations returning values.
- `mutateListVoid()` — Wrap generated list mutations without return values.

The mutation wrappers coordinate permission checks, previous-value snapshots, updates, and guard validation.

A mutation that violates a restriction is rejected.

Where supported, the Environment restores the previous value before propagating the error.

### 6.4 Runtime Scope Management

The Environment maintains a stack of scope frames.

`pushScope()` enters a new runtime scope, and `popScope()` removes it.

`ScopeGuard` provides RAII-based scope management.

When the generated C++ execution leaves a scope, its scope guard removes the corresponding runtime bindings and scoped guards.

This mechanism also supports cleanup during exception unwinding.

---

## 7. Locks and Guards

### 7.1 Lock and Unlock

Locking is associated with individual runtime variable bindings.

For example:

```gash
var x = 10;
lock x;
unlock x;
x = 20;
```

A locked variable remains readable, but controlled mutation operations cannot modify it until it is unlocked.

Locks also apply to controlled list operations, including element updates and resizing.

Checkpoint restoration checks lock restrictions before applying saved values.

Repeated locking of an already-locked variable or unlocking of an already-unlocked variable is treated as a safe no-op.

### 7.2 Mutation Guards

**Implementation:** `runtime/Guard.h`, `runtime/Guard.cpp`

Guards define conditions that must remain satisfied when program state changes.

For example:

```gash
var balance = 100;
guard balance >= 0;
```

`GuardManager` maintains registered guard rules.

The runtime supports predicate-based, typed, AST-expression, and live guard evaluation mechanisms.

A guard is checked when registered, and an initially invalid condition is rejected.

### 7.3 Mutation Validation and Rollback

During a controlled mutation, the runtime verifies applicable restrictions.

The Environment can register live guard callbacks using `addLiveGuard()`.

If a mutation causes an active guard condition to become false, the runtime attempts to restore the previous state and reports a guard violation.

Guards are associated with lexical scopes. When a scope ends, the relevant registrations are removed.

---

## 8. Checkpoints and Restoration

**Implementation:** `runtime/Checkpoint.h`, `runtime/Checkpoint.cpp`, `runtime/Environment.cpp`

Checkpoints preserve selected variable states for later restoration.

```gash
var x = 10;
checkpoint initial(x);
x = 100;
restore(initial);
output(x);
```

Expected output:

```text
10
```

### 8.1 CheckpointManager

`CheckpointManager` stores snapshots under checkpoint labels.

Checkpoint creation validates label and variable information.

Invalid cases include:

- Empty or duplicate checkpoint labels
- Empty checkpoint variable lists
- Duplicate variable names
- Missing or inconsistent snapshot information

A checkpoint is stored only after successful validation.

### 8.2 Variable Snapshots

Saved state includes:

- Variable name
- Current value
- Declared type
- Scope identifier
- Initialization state
- Binding identity information

Runtime value-copy semantics preserve independent snapshots, including list contents.

### 8.3 Restoration Process

Checkpoint restoration follows a staged process:

1. Synchronize current generated C++ values with runtime bindings.
2. Copy the current scope frames into a candidate state.
3. Validate variable existence, types, scope identities, binding identities, and locks.
4. Apply saved values to the candidate state.
5. Validate registered guards.
6. Commit the candidate state and write restored values back to generated C++ variables.
7. Validate live guards and roll back when necessary.

The Environment provides `restoreVariablesAtomically()` for coordinated restoration.

This design prevents ordinary validation failures from partially applying a checkpoint.

The implementation also supports rollback after live-guard failures, although complete exception safety still depends on the behaviour of the underlying write-back operations.

---

## 9. Execution Budgets

**Implementation:** `runtime/Budget.h`, `runtime/Budget.cpp`

Execution budgets limit the number of instrumented operations performed within a block.

```gash
budget (10) {
    output("Hello");
}
```

The generated program uses `BudgetScope` to manage active budgets.

The runtime tracks active budget scopes and decrements their remaining allowances when an instrumented operation executes.

Nested budgets are supported. Each active budget accounts for operations executed within its scope.

When an applicable budget is exhausted, the runtime raises a `BudgetExceeded` exception.

Budget scope management uses RAII to maintain the active budget stack and remove completed scopes.

Budget exhaustion has dedicated control-flow handling in generated code. It should therefore be distinguished from uncaught general runtime errors when interpreting test exit statuses.

---

## 10. Inspection and Preview

**Implementation:** `runtime/Inspect.h`, `runtime/Inspect.cpp`, `runtime/Preview.h`, `runtime/Preview.cpp`

GASH provides two complementary mechanisms for observing program state.

### 10.1 List Inspection

`inspect()` displays information about a typed list without modifying it.

Its output includes:

- Element type
- Logical size
- Capacity
- Indexed positions
- Initialized and uninitialized elements

For example:

```gash
var nums = list<int>(3);
nums[0] = 7;
nums[2] = 9;
inspect(nums);
```

### 10.2 Variable Preview

`preview()` displays information about active runtime variable bindings.

The Environment provides records containing variable names, types, values, lock status, and scope information.

The generated program registers its active Environment through `registerPreviewEnvironment()`.

The preview component then uses the registered provider to obtain state information without directly managing the Environment's internal storage.

This separation allows preview formatting to remain independent of variable-state management.

---

## 11. Compiler Pipeline and Command-Line Interface

**Implementation:** `include/CompilerPipeline.h`, `src/CompilerPipeline.cpp`, `src/main.cpp`

The command-line interface supports several compiler modes.

| Mode | Responsibility |
|---|---|
| `--tokens` | Perform lexical analysis and display tokens |
| `--check` | Perform lexical, syntax, and semantic validation |
| `--emit-cpp` | Produce generated C++17 source |
| `--run` | Generate, compile, and execute a GASH program |
| Default | Generate C++17 without execution |

### 11.1 Native Compilation

After source generation, the compiler pipeline invokes `g++` with the required include paths, runtime source files, and compilation options.

The generated executable is then launched as a separate process.

On POSIX platforms, the process execution layer uses operating-system process facilities. Windows-specific execution handling is also provided.

The pipeline captures process status and reports compilation or execution failures.

### 11.2 Repository and Runtime Path Resolution

The compiler requires access to GASH runtime headers and implementation files when compiling generated programs.

The pipeline contains repository-root discovery logic to locate these resources.

This supports relevant invocation scenarios in which the compiler is launched from outside the repository root, reducing reliance on one fixed working directory.

---

## 12. Testing Architecture

The test infrastructure includes component tests, end-to-end compilation tests, expected-error tests, and advanced compiler regression cases.

### 12.1 CTest

The project uses CMake and CTest to register automated tests.

The existing CMake configuration includes 27 tests covering lexer, parser, AST, semantic analysis, runtime components, code generation, integration, and example-based end-to-end execution.

The updated testing configuration adds `AdvancedPipelineRegressionTests` on supported POSIX platforms, bringing the total to **28 registered CTest tests**.

The advanced regression executable contains 15 focused cases and counts as one registered CTest test.

### 12.2 End-to-End Compilation Tests

The end-to-end tests exercise the complete compilation path:

```text
GASH source
    |
    v
Lexical + syntax + semantic validation
    |
    v
C++ generation
    |
    v
g++ compilation and linking
    |
    v
Native executable
    |
    v
Output and exit-status verification
```

The existing `E2EPipelineTests` verifies combinations of compiler features and runtime behaviour.

Seven additional named CTest cases verify example programs, including basic language constructs and checkpoint restoration.

The helper `tests/run_gash_example.cmake` executes selected examples and compares their actual output against expected output.

### 12.3 Unified Test Runner

**Implementation:** `tests/test_all.cpp`

The unified runner coordinates three stages:

1. **Registered CTest tests:** Executes the configured CTest suite.
2. **GASH CLI examples:** Runs 22 selected `.gash` programs, checking exact outputs, expected runtime errors, or successful execution.
3. **Negative compilation tests:** Runs six invalid source programs and verifies that `--check` rejects them with diagnostics.

The runner distinguishes intentionally failing programs from unexpected test failures.

Seven basic examples are execution smoke tests, meaning their output is not asserted exactly.

The unified runner invokes CTest itself and must not be registered as another CTest test, since that would cause recursive execution.

### 12.4 Advanced Pipeline Regression Tests

**Implementation:** `tests/test_advanced_pipeline.cpp`

The advanced C++17 regression runner tests 15 targeted scenarios, including:

- Global variables accessed within functions
- Boolean and list output formatting
- Division and modulo by zero
- Invalid exponentiation
- Negative repetition counts
- Character literal escaping
- Nested function declarations
- Typed-list function parameters
- Missing returns from non-void functions
- Runtime exception handling
- Syntax error locations
- Execution from outside the repository root

The runner creates temporary source files, compiles and executes them through GASH, and checks the resulting output, diagnostics, and exit status.

Temporary files are removed after the test run.

### 12.5 Build and Test Commands

From the repository root:

```bash
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

To run the unified pipeline:

```bash
c++ -std=c++17 tests/test_all.cpp -o build/test_all
./build/test_all
```

To run only the advanced regression suite:

```bash
ctest --test-dir build -R '^AdvancedPipelineRegressionTests$' --output-on-failure
```

**Verification history:** The previous compiler revision passed 27/27 CTest tests, 22/22 CLI cases, six negative compilation cases, and 15 targeted regression cases on macOS.

The newly integrated C++ regression runner compiles successfully, but its complete 15-case execution still requires confirmation on the final project revision. Once confirmed, the CTest suite will provide the corresponding 28-test result.

---

## 13. Design Summary

Transparent GASH combines a conventional source-to-source compiler with explicit runtime mechanisms for observing and controlling program state.

Its architecture emphasizes five design principles.

1. **Separation of concerns:** Lexing, parsing, semantic analysis, code generation, compilation, and runtime responsibilities are implemented in distinct components.

2. **Static correctness checks:** Type, declaration, and scope errors are detected before generating executable code.

3. **Controlled mutation:** Environment bindings coordinate locks, guards, and variable updates, including rollback where supported.

4. **Runtime transparency:** Checkpoints, budgets, inspection, and preview provide mechanisms for observing and constraining program behaviour.

5. **Execution safety:** Checked arithmetic, validated repetition counts, explicit missing-return failures, and runtime exception handling reduce unexpected behaviour in generated programs.

The design preserves familiar imperative programming constructs while integrating transparency features directly into the language.

For usage instructions and further details, see:

- [README](../README.md)
- [Grammar Reference](Grammar.md)
- [Language Reference](LanguageReference.md)
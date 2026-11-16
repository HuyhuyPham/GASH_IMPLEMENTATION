# Transparent GASH

**A transparency-first programming language implemented in C++17**

Transparent GASH is a programming language developed for the COMP2001 Programming Languages project. It is designed around the principle:

**“Nothing happens behind your back.”**

GASH combines familiar programming constructs with transparency-oriented features that make program state easier to inspect, control, and restore.

The language is implemented as a source-to-source compiler that translates `.gash` programs into C++17 code. Its runtime provides support for typed lists, mutation guards, explicit locking, checkpoints, state inspection, and execution budgets.

---

## 1. Project Overview

The GASH compiler follows a multi-stage compilation pipeline:

```text
GASH Source (.gash)
        |
        v
       Lexer
        |
        v
   Token Stream
        |
        v
      Parser
        |
        v
 Abstract Syntax Tree
        |
        v
  Semantic Analyzer
        |
        v
   Code Generator
        |
        v
 Generated C++17
        |
        v
 C++ Compiler (g++)
        |
        v
 Native Executable
        |
        v
   Program Output
```

Each compiler stage has a distinct responsibility:

1. **Lexical analysis:** Converts source code into tokens and identifies lexical errors.
2. **Parsing:** Constructs an Abstract Syntax Tree (AST) using recursive-descent parsing.
3. **Semantic analysis:** Validates types, declarations, variable scopes, function calls, and language-specific constraints.
4. **Code generation:** Translates validated AST constructs into C++17.
5. **Compilation and execution:** Compiles generated source using `g++` and executes the resulting program.

The generated programs use GASH runtime components to enforce language-specific behaviour, including mutation restrictions, variable-state management, checkpoints, and execution budgets.

### 1.1 Documentation

The repository contains three additional reference documents:

| Document | Description |
|---|---|
| [Grammar Reference](docs/Grammar.md) | Defines formal syntax, EBNF productions, operator precedence, and valid and invalid syntax examples. |
| [Language Reference](docs/LanguageReference.md) | Explains language features, type rules, control flow, built-in functions, and execution semantics. |
| [Compiler and Runtime Architecture](docs/Architecture.md) | Describes the compilation pipeline, component responsibilities, runtime Environment, guards, locks, checkpoints, and memory management. |

These documents complement the build, execution, and testing instructions provided in this README.

---

## 2. Requirements

The following tools are required:

- **CMake 3.16 or later** for project configuration and building.
- **A C++17-compatible compiler** for compiling the GASH implementation.
- **`g++` available in the system's `PATH`** for the GASH `--run` command.
- **CTest**, included with CMake, for automated testing.

The project uses C++17 and is configured through `CMakeLists.txt`.

The implementation includes platform-specific execution support for POSIX systems and Windows. The reported successful build and test results were obtained on macOS; other target platforms should be tested separately.

---

## 3. Building the Compiler

Open a terminal in the repository's root directory.

### 3.1 Configure the Project

```bash
cmake -S . -B build
```

This configures the project and generates its build files.

### 3.2 Build

```bash
cmake --build build
```

This builds the GASH compiler, its compiler/runtime support library, and the configured test executables.

On macOS and Linux, the compiler executable is expected at:

```text
build/gash
```

### 3.3 Clean and Rebuild

To clean and rebuild the project:

```bash
cmake --build build --target clean
cmake --build build
```

For a completely fresh configuration, remove the generated `build/` directory before running the configuration and build commands again.

---

## 4. Using the GASH Compiler

The GASH command-line interface provides several modes for inspecting, validating, generating, and executing programs.

### 4.1 Display Help

```bash
./build/gash --help
```

Displays the available command-line options.

### 4.2 Tokenize a Source File

```bash
./build/gash --tokens examples/checkpoint_smoke.gash
```

This performs lexical analysis and displays recognized tokens, including their types, lexemes, line numbers, and column numbers.

The tokenization mode does not perform parsing or semantic analysis.

### 4.3 Validate a Source File

```bash
./build/gash --check examples/checkpoint_smoke.gash
```

This command performs lexical analysis, parsing, and semantic analysis without compiling or executing the generated C++.

On successful validation, the compiler prints:

```text
Lexical, syntax, and semantic checks passed.
```

Errors are reported if the program violates lexical, syntactic, or semantic rules.

### 4.4 Generate C++17 Source

```bash
./build/gash --emit-cpp examples/checkpoint_smoke.gash
```

This validates the program and generates the corresponding C++17 source file.

By default, the output is placed beside the input:

```text
examples/checkpoint_smoke.cpp
```

A custom output path may also be supplied:

```bash
./build/gash --emit-cpp examples/checkpoint_smoke.gash build/generated.cpp
```

This mode generates C++ source but does not execute it.

### 4.5 Compile and Execute a Program

```bash
./build/gash --run examples/checkpoint_smoke.gash
```

The `--run` mode performs the complete compilation and execution process:

1. Reads the GASH source file.
2. Performs lexical and syntax analysis.
3. Performs semantic validation.
4. Generates C++17 source code.
5. Compiles the generated source using `g++ -std=c++17`.
6. Links the required GASH runtime implementation files.
7. Executes the compiled program.
8. Returns the program's exit status.

By default, generated files are placed beside the input source file:

```text
examples/checkpoint_smoke.gash
examples/checkpoint_smoke.cpp
examples/checkpoint_smoke.out
```

On Windows, the generated executable uses the `.exe` extension.

**Note:** The `--run` command may overwrite existing generated files at the corresponding output paths.

### 4.6 Default Command

```bash
./build/gash examples/checkpoint_smoke.gash
```

Without an explicit mode flag, GASH validates the program and generates C++ source without executing it.

This is equivalent to `--emit-cpp` without a custom output path.

### 4.7 Example Program: Checkpoint and Restore

The following example demonstrates GASH's checkpoint and restoration functionality.

**Source:** `examples/checkpoint_smoke.gash`

```gash
var x = 10;

checkpoint start(x);

x = 100;

restore(start);

output(x);
```

The program saves the original value of `x`, modifies it, and then restores the saved value.

Run the example:

```bash
./build/gash --run examples/checkpoint_smoke.gash
```

**Expected output:**

```text
10
```

This example is also covered by the `CheckpointSmokeE2E` automated test.

---

## 5. Language Features

GASH combines statically typed imperative programming with explicit runtime transparency mechanisms.

For complete syntax and semantics, refer to the [Language Reference](docs/LanguageReference.md).

### 5.1 Variables and Static Types

Variables are declared using `var` and require an initializer.

```gash
var age = 20;
var price = 15.5;
var name = "Alice";
var active = true;
```

Supported primitive types are:

- `int`
- `float`
- `bool`
- `char`
- `string`

Variable types are inferred from their initializers and remain fixed throughout their lifetimes.

GASH uses strict type checking and supports explicit type conversions.

```gash
var x = 10;
var y = float(x);
var text = string(x);
```

### 5.2 Expressions and Control Flow

GASH supports:

- Arithmetic, comparison, and logical expressions
- Conditional execution using `if` and `otherwise`
- `while` and `repeat` loops
- Loop control using `exit` and `skip`
- Functions with explicitly typed parameters and inferred return types
- Recursive functions

Blocks are enclosed in braces, and statements are terminated with semicolons.

### 5.3 Typed Lists

GASH provides typed lists, including:

```gash
var numbers = list<int>(3);

numbers[0] = 10;
numbers[1] = 20;

output(numbers[0]);
```

Lists support logical size, capacity, initialized and uninitialized positions, bounds checking, element assignment, resizing, and independent value-copy semantics.

The underlying list implementation uses `std::vector<std::optional<T>>`, allowing it to distinguish initialized elements from uninitialized positions.

List sizes can be modified using `extend()`.

### 5.4 List Inspection — `inspect()`

The `inspect()` function provides a read-only view of a list's state.

```gash
var values = list<int>(2);
values[0] = 7;

inspect(values);
```

Inspection information includes element type, logical size, capacity, initialized positions, and stored values.

Uninitialized positions are identified separately.

### 5.5 Variable Preview — `preview()`

The `preview()` function displays information about active runtime variables.

```gash
var x = 10;
preview();
```

The generated program registers its runtime Environment as the preview provider.

The Environment supplies information about variable names, declared types, values, lock states, and lexical scopes.

### 5.6 Mutation Guards

Guards define conditions that restrict permitted changes to program state.

```gash
var balance = 100;
guard balance >= 0;
```

Mutations are validated against applicable guards.

If an invalid modification violates a guard, the runtime rejects the modification, restores the previous value through its rollback mechanism, and reports an error.

### 5.7 Lock and Unlock

GASH provides explicit control over variable mutability.

```gash
var count = 10;

lock count;
unlock count;

count = 20;
```

A locked variable remains readable but cannot be modified through controlled runtime mutation operations until it is unlocked.

Locks also restrict list element modifications, list resizing, and restoration of locked variables.

### 5.8 Checkpoints and Restoration

Checkpoints save selected variable values under a named label.

```gash
var x = 10;

checkpoint initial(x);

x = 20;

restore(initial);

output(x);
```

**Expected output:**

```text
10
```

Checkpoint snapshots preserve variable information needed for restoration, including types, scope information, and independent copies of saved values.

Restoration validates applicable variable identities, lock restrictions, and guards before committing restored state.

### 5.9 Execution Budgets

Execution budgets restrict the number of counted operations performed within a designated block.

```gash
budget (10) {
    output("Hello");
}
```

The generated program uses `BudgetScope` to track operation allowances.

Nested budgets are supported. If a budget is exhausted, the runtime raises a `BudgetExceeded` exception.

### 5.10 Automatic Resource Management

GASH lists use C++ RAII-based ownership.

List-owned storage is released automatically when the corresponding objects are destroyed.

This includes normal scope exit and applicable exception unwinding.

GASH programs do not require an explicit `cleanup()` function for list-owned resources.

---

## 6. Automated Testing

The project uses CTest for automated testing across the compiler and runtime components.

### 6.1 Run All Tests

After building the project:

```bash
ctest --test-dir build --output-on-failure
```

For detailed test output:

```bash
ctest --test-dir build -V
```

To list all registered tests without executing them:

```bash
ctest --test-dir build -N
```

### 6.2 Registered Test Suites

The reported CMake configuration includes 22 registered tests.

| Area | CTest test names |
|---|---|
| Lexical analysis | `LexerTests` |
| AST and parsing | `AstTests`, `ParserTests`, `StatementParserTests`, `LexerParserTests`, `ParserErrorTests`, `ParserNestedTests` |
| Semantic analysis | `SemanticTests` |
| Guards and Environment | `GuardLockEnvironmentTests` |
| Checkpoints | `CheckpointTests`, `CheckpointRuntimeTests`, `CodegenCheckpointTests` |
| Lists | `ListTests` |
| Inspection and preview | `InspectPreviewTests` |
| Automatic cleanup | `AutoCleanupTests` |
| Code generation | `CodegenTests`, `S2CodegenTests`, `S3CodegenTests` |
| Integration & E2E pipeline | `IntegrationTests`, `E2EPipelineTests` |
| End-to-end `.gash` examples | `HelloE2E`, `ConditionsE2E`, `FunctionsE2E`, `LoopsE2E`, `CheckpointSmokeE2E`, `CheckpointMultiE2E`, `CheckpointListE2E` |

**Verified test results:** The latest reported macOS build passed all 22 registered CTest tests (100%), including the three checkpoint end-to-end tests.

These results describe the tested project revision and do not constitute verification of every possible GASH program or target platform.

### 6.3 Run Selected Tests

Run lexer tests:

```bash
ctest --test-dir build -R '^LexerTests$' --output-on-failure
```

Run list-related tests:

```bash
ctest --test-dir build -R 'ListTests|InspectPreviewTests|AutoCleanupTests' --output-on-failure
```

Run checkpoint and E2E pipeline tests:

```bash
ctest --test-dir build -R 'Checkpoint|E2EPipeline' --output-on-failure
```

### 6.4 End-to-End Tests

Three automated end-to-end tests execute checkpoint-based GASH programs through the compiler.

| CTest name | Source file | Expected output |
|---|---|---|
| `HelloE2E` | `examples/hello.gash` | `Hello, GASH!` |
| `ConditionsE2E` | `examples/conditions.gash` | `High Distinction`, `Verified` |
| `FunctionsE2E` | `examples/functions.gash` | `30`, `120` |
| `LoopsE2E` | `examples/loops.gash` | `1`, `3`, `4`, `repeat step`, `repeat step`, `repeat step` |
| `CheckpointSmokeE2E` | `examples/checkpoint_smoke.gash` | `10` |
| `CheckpointMultiE2E` | `examples/checkpoint_multi.gash` | `10`, `20`, `10`, `20` |
| `CheckpointListE2E` | `examples/checkpoint_list.gash` | `7`, `8` |

For multiple output values, each value appears on a separate line.

These tests use `tests/run_gash_example.cmake` to execute the relevant GASH program and compare actual output against the expected result.

They verify complete compilation and execution paths, complementing the unit and integration test suites.

---

## 7. Project Structure

The repository is organized into compiler, runtime, testing, example, and documentation directories.

```text
GASH_IMPLEMENTATION/
├── CMakeLists.txt
├── README.md
├── include/
│   ├── AST.h
│   ├── Token.h
│   ├── Lexer.h
│   ├── Parser.h
│   ├── Type.h
│   ├── Semantic.h
│   ├── CodeGenerator.h
│   └── CompilerPipeline.h
├── src/
│   ├── main.cpp
│   ├── Lexer.cpp
│   ├── Parser.cpp
│   ├── Semantic.cpp
│   ├── CodeGenerator.cpp
│   └── CompilerPipeline.cpp
├── runtime/
│   ├── Value.h
│   ├── List.h
│   ├── Environment.h
│   ├── Environment.cpp
│   ├── Guard.h
│   ├── Guard.cpp
│   ├── Checkpoint.h
│   ├── Checkpoint.cpp
│   ├── Budget.h
│   ├── Budget.cpp
│   ├── Inspect.h
│   ├── Inspect.cpp
│   ├── Preview.h
│   └── Preview.cpp
├── tests/
│   ├── test_lexer.cpp
│   ├── test_parser.cpp
│   ├── test_semantic.cpp
│   ├── test_list.cpp
│   ├── test_inspect_preview.cpp
│   ├── test_auto_cleanup.cpp
│   ├── test_integration.cpp
│   ├── run_gash_example.cmake
│   └── ... additional test files
├── examples/
│   ├── hello.gash
│   ├── variables.gash
│   ├── conditions.gash
│   ├── loops.gash
│   ├── functions.gash
│   ├── lists.gash
│   ├── transparency.gash
│   ├── checkpoint_smoke.gash
│   ├── checkpoint_multi.gash
│   └── checkpoint_list.gash
└── docs/
    ├── Grammar.md
    ├── LanguageReference.md
    └── Architecture.md
```

This is a simplified directory overview rather than an exhaustive file listing.

CMake generates build outputs inside `build/`, which is not part of the source directory structure.

---

## 8. Error Handling

GASH reports errors during lexical analysis, parsing, semantic analysis, generated C++ compilation, and program execution.

### 8.1 Lexical Errors

Lexical diagnostics identify the source location of invalid tokens or malformed literals.

Reported information includes:

- Source filename
- Line number
- Column number
- Error description

### 8.2 Syntax and Semantic Errors

Syntax errors are detected by the parser, while semantic errors are identified during type checking and declaration validation.

Examples include malformed statements, incompatible assignments, undeclared variables, invalid function calls, and inappropriate statement contexts.

### 8.3 Runtime Errors

Runtime validation handles conditions such as:

- List index violations
- Access to uninitialized list elements
- Lock violations
- Guard violations
- Invalid checkpoint restoration
- Execution budget exhaustion
- Invalid runtime conversions

Invalid programs and failed compiler operations return non-zero exit statuses where applicable.

Compilation failures and runtime errors are reported through the command-line interface or generated program diagnostics.

---

## 9. Development Team

| Member | Main responsibilities |
|---|---|
| Quynh Anh Tran | Lexer, lexical errors, CLI/CMake, lists, inspect, preview, automatic cleanup tests, and documentation |
| Gege Xin | AST, parser, grammar, checkpoints, and restoration |
| Nguyen Dang Huy Pham | Semantic analysis, type system, Environment, guards, and locks |
| Le Bao Uyen Truong | Code generation, execution budgets, compilation pipeline, and integration testing |

---

## 10. Additional References

For more detailed technical information:

- [Grammar Reference](docs/Grammar.md) — Formal grammar and syntax restrictions.
- [Language Reference](docs/LanguageReference.md) — Complete descriptions of language behaviour and built-in operations.
- [Compiler and Runtime Architecture](docs/Architecture.md) — Internal compiler design, runtime components, and state-management mechanisms.

The compiler and runtime source files provide the implementation corresponding to these documents.
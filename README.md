# Transparent GASH

**A transparency-first programming language implemented in C++17**

Transparent GASH is a custom programming language developed for the COMP2001 Programming Languages project. It is designed around the principle:

**“Nothing happens behind your back.”**

GASH combines familiar programming constructs with features that make program state easier to inspect, control, and restore. The language is implemented as a source-to-source compiler that translates GASH programs into C++17.

## 1. Project Overview

The GASH compiler processes source files through the following stages:

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
 Generated C++ (.cpp)
        |
        v
  g++ (C++17 Compiler)
        |
        v
  Executable Program
```

The implementation also includes runtime components for typed lists, execution budgets, guards, variable environments, checkpoints, and state inspection.

## 2. Requirements

To build and run the project, install:

- CMake 3.16 or later
- A C++ compiler supporting C++17
- `g++` available in the system's `PATH` for GASH's `--run` command
- CTest, included with CMake

The project uses the C++17 standard and is configured with CMake.

The CLI includes platform-specific process execution code for Windows and POSIX systems. The build and test instructions below should be verified on each target platform before claiming full cross-platform support.

## 3. Building the Compiler

Open a terminal in the project's root directory.

### Step 1: Configure the project

```bash
cmake -S . -B build
```

### Step 2: Build

```bash
cmake --build build
```

This builds the GASH compiler, shared compiler/runtime library, and configured test executables.

On macOS and Linux with the current CMake configuration, the compiler executable is:

```text
build/gash
```

### Clean and rebuild

```bash
cmake --build build --target clean
cmake --build build
```

To force a fresh CMake configuration, remove the generated `build/` directory and configure the project again.

## 4. Using the GASH Compiler

The command-line interface supports token inspection, lexical/syntax/semantic validation, C++ generation, and direct compilation and execution.

### 4.1 Display help

```bash
./build/gash --help
```

### 4.2 Tokenize a source file

```bash
./build/gash --tokens examples/checkpoint_smoke.gash
```

The lexer prints each recognized token with its type, lexeme, line number, and column number.

This mode performs lexical analysis without continuing to parsing or semantic validation.

### 4.3 Check a source file

```bash
./build/gash --check examples/checkpoint_smoke.gash
```

This mode performs:

1. Lexical analysis
2. Syntax analysis
3. Semantic analysis

On successful validation, the CLI prints:

```text
Lexical, syntax, and semantic checks passed.
```

This command does not compile or execute the resulting C++ program.

### 4.4 Generate C++ code

```bash
./build/gash --emit-cpp examples/checkpoint_smoke.gash
```

By default, the generated C++ file is written beside the source file, using the same filename with a `.cpp` extension:

```text
examples/checkpoint_smoke.cpp
```

To specify a different output path:

```bash
./build/gash --emit-cpp examples/checkpoint_smoke.gash build/generated.cpp
```

The compiler performs lexical, syntax, and semantic analysis before generating the C++ source.

### 4.5 Compile and run a GASH program

```bash
./build/gash --run examples/checkpoint_smoke.gash
```

This mode performs the full compilation pipeline:

1. Reads the GASH source file.
2. Tokenizes and parses the program.
3. Performs semantic analysis.
4. Generates the corresponding C++ source.
5. Compiles the generated C++ with `g++ -std=c++17`.
6. Links the required GASH runtime implementation files.
7. Executes the resulting program.
8. Returns the executable's exit status.

By default, the generated files use the source filename:

```text
examples/checkpoint_smoke.gash
examples/checkpoint_smoke.cpp
examples/checkpoint_smoke.out
```

On Windows, the generated executable uses the `.exe` extension.

**Note:** `--run` writes generated files beside the input file and may overwrite existing files with the same generated paths.

### 4.6 Default command

```bash
./build/gash examples/checkpoint_smoke.gash
```

Without an explicit flag, the compiler generates C++ source but does not execute the program.

This is equivalent to using `--emit-cpp` without a custom output path.

## 5. Language Features

### 5.1 Variables and static types

Variables use the `var` keyword and require an initializer.

```text
var age = 20;
var price = 15.5;
var name = "Alice";
var active = true;
```

The language infers a variable's type from its initializer. Variables retain their established type.

### 5.2 Expressions and control flow

GASH is designed to support arithmetic, comparison, and logical expressions, along with conditional statements, loops, and functions.

The language uses braces and semicolons for program structure.

Loop-control keywords include `exit` and `skip`, corresponding to the traditional concepts of breaking and continuing loops.

### 5.3 Typed lists

GASH provides typed lists, with runtime support for:

- Logical size and allocated capacity
- Initialized and uninitialized positions
- Bounds checking
- Element access and assignment
- Resizing
- Independent copies

The list implementation uses `std::vector<std::optional<T>>`, allowing the runtime to distinguish initialized elements from uninitialized slots.

### 5.4 `inspect()`

The `inspect()` feature displays list metadata, including:

- Element type
- Logical size
- Allocated capacity
- Element positions
- Initialized values and uninitialized slots

Inspection does not modify the list.

### 5.5 `preview()`

The `preview()` feature is designed to display the current runtime variable state.

Its runtime interface supports variable names, types, values, locking status, and scope information.

The implementation obtains variable snapshots through a registered provider, so end-to-end operation depends on integration with the runtime Environment.

### 5.6 Guards

Guards constrain the permitted values of variables.

For example:

```text
var balance = 100;
guard balance >= 0;
```

According to the language specification, a guard violation rejects the invalid modification and terminates execution.

### 5.7 Lock and unlock

GASH provides explicit control over variable mutability:

```text
var count = 10;
lock count;
unlock count;
count = 20;
```

A locked variable cannot be modified until its lock is released, subject to the language's mutation rules.

### 5.8 Checkpoints and restoration

Checkpoints support saving selected program state and restoring it later.

```text
var x = 10;
checkpoint initial(x);
x = 20;
restore(initial);
```

The implementation includes checkpoint runtime support and dedicated checkpoint tests.

Restoration must preserve state consistency and respect the applicable mutation and scope rules.

### 5.9 Execution budgets

Execution budgets provide a mechanism for limiting operations within a designated section of code.

The compiler and runtime include dedicated budget components.

### 5.10 Automatic resource cleanup

Typed lists use C++ RAII-based ownership.

Resources owned by list objects are automatically released when those objects are destroyed. This includes normal scope exit and C++ exception unwinding.

GASH programmers do not need to call a separate `cleanup()` function.

This behaviour is tested using the dedicated automatic-cleanup test suite.

## 6. Testing

The project uses CTest for automated testing.

### 6.1 Run all tests

After building the project:

```bash
ctest --test-dir build --output-on-failure
```

To display more detailed output:

```bash
ctest --test-dir build -V
```

To list registered tests without running them:

```bash
ctest --test-dir build -N
```

### 6.2 Test suites

The current `CMakeLists.txt` registers 22 tests.

| Area | CTest test names |
|---|---|
| Lexical analysis | `LexerTests` |
| AST and parsing | `AstTests`, `ParserTests`, `StatementParserTests`, `LexerParserTests`, `ParserErrorTests`, `ParserNestedTests` |
| Semantic analysis | `SemanticTests` |
| Guards and environment | `GuardLockEnvironmentTests` |
| Checkpoints | `CheckpointTests`, `CheckpointRuntimeTests`, `CodegenCheckpointTests` |
| Lists | `ListTests` |
| Inspection and preview | `InspectPreviewTests` |
| Automatic cleanup | `AutoCleanupTests` |
| Code generation | `CodegenTests`, `S2CodegenTests`, `S3CodegenTests` |
| Integration | `IntegrationTests` |
| End-to-end checkpoint examples | `CheckpointSmokeE2E`, `CheckpointMultiE2E`, `CheckpointListE2E` |

These are registered tests, not a claim that all tests currently pass.

### 6.3 Run specific tests

Run the lexer tests:

```bash
ctest --test-dir build -R '^LexerTests$' --output-on-failure
```

Run list-related tests:

```bash
ctest --test-dir build -R 'ListTests|InspectPreviewTests|AutoCleanupTests' --output-on-failure
```

Run checkpoint tests:

```bash
ctest --test-dir build -R 'Checkpoint' --output-on-failure
```

### 6.4 End-to-end tests

Three CTest cases execute checkpoint-based GASH examples through the compiler:

| Test | Source file | Expected output lines |
|---|---|---|
| `CheckpointSmokeE2E` | `examples/checkpoint_smoke.gash` | `10` |
| `CheckpointMultiE2E` | `examples/checkpoint_multi.gash` | `10`, `20`, `10`, `20` |
| `CheckpointListE2E` | `examples/checkpoint_list.gash` | `7`, `8` |

These tests are registered using `tests/run_gash_example.cmake`.

They are intended to verify compilation and execution, rather than only individual compiler components.

## 7. Project Structure

```text
GASH_IMPLEMENTATION/
├── CMakeLists.txt
├── README.md
├── include/
│   ├── Lexer.h
│   ├── Parser.h
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
│   ├── List.h
│   ├── Budget.cpp
│   ├── Checkpoint.cpp
│   ├── Guard.cpp
│   ├── Environment.cpp
│   ├── Inspect.cpp
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
│   ├── checkpoint_smoke.gash
│   ├── checkpoint_multi.gash
│   └── checkpoint_list.gash
├── docs/
│   └── Grammar.md
└── build/
    └── Generated build outputs
```

This is a simplified structural overview rather than an exhaustive file listing.

## 8. Error Handling

The CLI handles errors at multiple stages.

**Lexical errors** include the source filename, line number, and column number, followed by a diagnostic message.

Other errors are reported using the prefix:

```text
GASH error:
```

The compiler returns a non-zero status for invalid arguments and compiler errors.

During `--run`, errors may originate from GASH analysis, generated C++ compilation, or execution of the compiled program.

The compilation pipeline wraps failures with messages identifying the stage at which they occurred.

## 9. Development Team

| Member | Main responsibilities |
|---|---|
| Quynh Anh Tran (Atq) | Lexer, lexical errors, CLI/CMake, lists, inspect, preview, automatic cleanup tests, and documentation |
| Gege Xin (Gloria) | AST, parser, grammar, checkpoints, and restoration |
| Nguyen Dang Huy Pham (Hihi) | Semantic analysis, type system, Environment, guards, and locks |
| Le Bao Uyen Truong (Sala) | Code generation, execution budgets, compilation pipeline, and integration testing |

## 10. Implementation and Integration Notes

The compiler is structured into separate lexical, syntactic, semantic, code-generation, and runtime components.

The `gash_core` CMake library contains the compiler implementation and supporting runtime source files. The CLI executable links against this library.

For direct execution, `CompilerPipeline` generates C++ and invokes `g++` with the runtime source files needed by the generated program.

The source-to-source design allows GASH to reuse the C++17 type system, runtime facilities, and standard memory-management mechanisms.

Final submission testing should include a clean build, the full CTest suite, and end-to-end execution of representative valid and invalid GASH programs.
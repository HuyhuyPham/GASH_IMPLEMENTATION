# Transparent GASH

**Github link**: https://github.com/HuyhuyPham/GASH_IMPLEMENTATION

**A transparency-first programming language implemented in C++17**
*COMP2001 Programming Languages project*

> Nothing happens behind your back.

Transparent GASH is a statically typed programming language that makes state changes visible and controllable. It provides familiar expressions, functions, branches, and loops alongside typed lists, mutation guards, locks, checkpoints, inspection, and execution budgets.

GASH is a **source-to-source compiler**: it analyses `.gash` source code, emits C++17, and uses `g++` to create an executable. Its runtime library implements the language's state-management and safety features.

## 1. Compiler overview

```text
.gash source
    |
    v
Lexer -> tokens -> Parser -> AST -> Semantic analyzer
                                      |
                                      v
                                 Code generator
                                      |
                                      v
                                 C++17 source
                                      |
                                      v
                                g++ + GASH runtime
                                      |
                                      v
                               Native executable
```

The lexer identifies tokens and source positions. The recursive-descent parser builds an abstract syntax tree (AST). Semantic analysis checks types, declarations, scopes, and function calls. Code generation translates validated constructs into C++17, which uses the GASH runtime for lists, guards, locks, checkpoints, budgets, and state inspection.

Further documentation:

| Document | Contents |
|---|---|
| [Grammar](docs/Grammar.md) | EBNF, statements, expressions, precedence, and syntax examples |
| [Language Reference](docs/LanguageReference.md) | Types, functions, lists, control flow, and language semantics |
| [Architecture](docs/Architecture.md) | Compiler phases, generated C++, runtime design, and state management |

## 2. Requirements

- **C++17-compatible compiler** and **CMake 3.16+** to build the project.
- **`g++` on `PATH`** to compile generated GASH programs with `--run`.
- **CTest** (supplied with CMake) for the registered test suite.
- **macOS or Linux** for the unified `test_all.cpp` runner and the advanced regression runner, which use POSIX facilities. The main compiler contains Windows-specific execution support, but the commands below are verified for the POSIX test workflow.

The original team test runs were performed on macOS. The program may also build on other platforms, but do not assume identical test-runner support without checking the toolchain.

## 3. Build

Open a terminal in the repository root:

```bash
cmake -S . -B build
cmake --build build
```

This builds the `gash` CLI, the shared compiler/runtime library, and the CTest executables. On macOS/Linux, the main executable is `build/gash`.

For a clean rebuild using the existing CMake configuration:

```bash
cmake --build build --clean-first
```

To create a completely separate clean build directory:

```bash
cmake -S . -B build-clean
cmake --build build-clean
ctest --test-dir build-clean --output-on-failure
```

The standalone unified runner in Section 6 specifically expects `build/gash` and the default `build/` directory; a `build-clean` build is for independent CMake/CTest verification.

### 3.1 Alternative build without CMake

For a POSIX machine with `g++` installed, the compiler can also be built directly:

```bash
g++ -std=c++17 -Iinclude -Iruntime src/*.cpp runtime/*.cpp -o gash
```

Use `./gash` rather than `./build/gash` for the resulting executable. This option builds the compiler CLI, not the CMake-defined test executables. Because `--run` invokes `g++` for generated programs, `g++` must still be on `PATH`.

## 4. Using the CLI

The examples below assume the repository-root working directory and the default CMake build.

| Command | Effect |
|---|---|
| `./build/gash --help` | Show supported CLI options |
| `./build/gash --tokens examples/hello.gash` | Lexical analysis; print tokens and locations |
| `./build/gash --check examples/hello.gash` | Lexing, parsing, and semantic validation only |
| `./build/gash --emit-cpp examples/hello.gash` | Generate C++17 source, without executing it |
| `./build/gash --emit-cpp examples/hello.gash build/hello.cpp` | Generate C++17 at a specified path |
| `./build/gash --run examples/hello.gash` | Generate, compile, and execute |
| `./build/gash examples/hello.gash` | Default: generate C++17 without execution |

Successful `--check` output is:

```text
Lexical, syntax, and semantic checks passed.
```

`--check` does **not** establish whether the generated C++ compiles; use `--run` for an end-to-end check. `--run` writes generated `.cpp` and executable files beside the `.gash` source by default (`.out` on macOS/Linux and `.exe` on Windows). Existing generated files at these paths may be overwritten.

### 4.1 Minimal checkpoint example

`examples/checkpoint_smoke.gash`:

```gash
var x = 10;
checkpoint start(x);
x = 100;
restore(start);
output(x);
```

Run:

```bash
./build/gash --run examples/checkpoint_smoke.gash
```

Expected output:

```text
10
```

### 4.2 Test your own program

Create `my_program.gash`, then run:

```bash
./build/gash --tokens my_program.gash
./build/gash --check my_program.gash
./build/gash --run my_program.gash
```

The first command helps diagnose lexical issues; the second checks program validity; the third exercises code generation, native compilation, and runtime execution.

## 5. Language features

### 5.1 Variables, types, and output

Variables require an initializer; their types are inferred and remain fixed.

```gash
var age = 20;
var price = 15.5;
var name = "Alice";
var enabled = true;
output(enabled);  # prints true
```

The primitive types are `int`, `float`, `bool`, `char`, and `string`. Explicit conversions include `int(...)`, `float(...)`, and `string(...)`. `output()` prints booleans as `true`/`false` and can display typed lists in a structured form.

### 5.2 Expressions and control flow

GASH supports arithmetic, comparisons, logical operators, `if`/`otherwise`, `while`, `repeat`, `exit`, and `skip`. Semicolons terminate simple statements, and braces delimit blocks.

Functions use explicit parameter types and inferred return types. Recursive functions, nested function declarations, and `list<T>` parameters are supported. Functions can refer to appropriately visible global variables and, for nested functions, enclosing local bindings.

Operations such as division/modulo by zero, invalid exponentiation, or a negative `repeat` count report runtime errors instead of silently producing invalid results. A non-void function reaching its end without a value also raises a runtime error.

### 5.3 Typed lists

```gash
var nums = list<int>(3);
nums[0] = 7;
nums[2] = 9;
inspect(nums);
```

Lists have a logical size, capacity, and initialized or uninitialized positions. The runtime checks bounds and invalid element access. Lists use value-copy semantics; `extend()` changes their size. List parameters can be declared as `list<int>`, `list<float>`, etc., for supported element types. See [Language Reference](docs/LanguageReference.md) for details.

### 5.4 Transparency and state management

| Feature | Purpose |
|---|---|
| `inspect(list)` | Display list type, size, capacity, and element initialization state |
| `preview()` | Display runtime variable values, types, lock status, and scope information |
| `guard` | Define a condition that assignments must preserve |
| `lock` / `unlock` | Block or permit mutation of a variable (including controlled list mutations) |
| `checkpoint` / `restore` | Save selected variable values and restore them subject to applicable checks |
| `budget (...) { ... }` | Apply an operation allowance to a block, including nested budgets |

For example:

```gash
var balance = 100;
guard balance >= 0;
checkpoint initial(balance);
balance = 75;
restore(initial);
output(balance);
```

Expected output: `100`.

List storage is managed through C++ RAII; GASH does not require an explicit cleanup function for list-owned resources.

## 6. Testing

Testing is divided into **registered CTest tests**, **CLI example and negative-source checks**, and **targeted advanced regression cases**. The command in Section 6.2 runs all of these through one C++ entry point on macOS/Linux.

### 6.1 Registered CTest tests

```bash
ctest --test-dir build --output-on-failure
```

You can list registered test names with `ctest --test-dir build -N` or run a subset with `ctest --test-dir build -R 'Checkpoint|E2E' --output-on-failure`.

**CTest registration:** 27 original tests plus `AdvancedPipelineRegressionTests` on macOS/Linux = **28 registered tests** in this updated package. On platforms where the POSIX-only regression runner is not built, the count remains 27.

| Test area | Registered tests |
|---|---|
| Lexing and parsing | `LexerTests`, `AstTests`, `ParserTests`, `StatementParserTests`, `LexerParserTests`, `ParserErrorTests`, `ParserNestedTests` |
| Semantic and runtime state | `SemanticTests`, `GuardLockEnvironmentTests`, `CheckpointTests`, `CheckpointRuntimeTests`, `ListTests`, `InspectPreviewTests`, `AutoCleanupTests` |
| Code generation and integration | `CodegenCheckpointTests`, `CodegenTests`, `S2CodegenTests`, `S3CodegenTests`, `IntegrationTests`, `E2EPipelineTests` |
| Example E2E | `HelloE2E`, `ConditionsE2E`, `FunctionsE2E`, `LoopsE2E`, `CheckpointSmokeE2E`, `CheckpointMultiE2E`, `CheckpointListE2E` |
| Additional POSIX regression | `AdvancedPipelineRegressionTests` (15 cases; one registered CTest test) |

The seven named example E2E tests use `tests/run_gash_example.cmake` to run source-to-executable checks and compare expected output. `E2EPipelineTests` performs further compiler integration checks. `AdvancedPipelineRegressionTests` runs 15 temporary source programs to verify compiler edge cases and runtime diagnostics.

Previously observed test results for the source revision before this additional C++ registration were **27/27 CTest passed**, **22/22 CLI examples passed**, **6/6 invalid-source checks passed**, and **15/15 targeted regression checks passed** on macOS. The newly registered 28-test CTest configuration should be rerun in the final submission environment; past results are not a substitute for that run.

### 6.2 Unified test runner (`test_all.cpp`)

The standalone `tests/test_all.cpp` invokes CTest and then executes additional tests using `build/gash`. Compile it from the repository root:

```bash
c++ -std=c++17 tests/test_all.cpp -o build/test_all
./build/test_all
```

Its stages are:

1. **CTest:** all registered CTest tests (28 on POSIX with the new regression test).
2. **GASH CLI examples:** 22 selected `.gash` programs, including output assertions, expected runtime failures, and seven execution smoke tests.
3. **Negative compilation:** six intentionally invalid sources in `tests/negative/`, each expected to fail during `--check` with a diagnostic.

`examples/cli_test.gash` is a development example and is skipped by default; include it with `./build/test_all --include-cli-test`. The seven smoke tests assert successful execution but do not compare exact output. Some CTest E2E checks overlap with CLI examples; execution totals are **not** a count of unique behaviours.

**Do not register `test_all.cpp` as a CTest test:** it invokes CTest itself and would create recursive testing. It is intentionally a separate launcher.

### 6.3 Advanced compiler regression runner (`test_advanced_pipeline.cpp`)

This is the C++17 replacement for the previous standalone script. It requires no Python installation. The 15 cases cover global access from functions, boolean/list output, invalid arithmetic, negative repetition, character escapes, nested functions, list parameters, missing return, runtime diagnostics, syntax locations, and execution outside the repository root.

It is built by CMake and registered automatically as `AdvancedPipelineRegressionTests` on macOS/Linux:

```bash
ctest --test-dir build -R '^AdvancedPipelineRegressionTests$' --output-on-failure
```

Or run its executable directly:

```bash
./build/test_advanced_pipeline ./build/gash .
```

Its `.gash` inputs, generated `.cpp` files, executables, and logs reside in a temporary directory that is removed after the tests. This C++ runner and `test_all.cpp` are separate: the advanced runner is called *by CTest*, and the unified runner calls CTest.

### 6.4 Negative compilation examples

The six invalid programs under `tests/negative/` test rejection of an undefined variable, incompatible assignment, duplicate declaration, missing semicolon, wrong function argument count, and malformed expression. These tests are checked with `--check`, not `--run`.

To inspect one error manually:

```bash
./build/gash --check tests/negative/undefined_variable.gash
```

A nonzero exit status and useful diagnostic are expected. These negative cases represent **successful error detection**, not failed tests.

## 7. Repository structure

```text
GASH_IMPLEMENTATION/
├── CMakeLists.txt
├── README.md
├── include/                       # Lexer, parser, AST, semantic, codegen headers
├── src/                           # Compiler implementations and CLI entry point
├── runtime/                       # Lists, environment, guards, budgets, checkpoints
├── examples/                      # Basic, checkpoint, and advanced .gash examples
├── tests/
│   ├── test_all.cpp               # Single launcher for CTest + CLI + negative tests
│   ├── test_advanced_pipeline.cpp # 15 POSIX compiler regression checks
│   ├── test_e2e_pipeline.cpp      # Existing CTest compiler E2E suite
│   ├── run_gash_example.cmake     # Exact-output example helper
│   ├── negative/                  # Invalid .gash source programs
│   └── ...                        # Unit and integration tests
└── docs/
    ├── Grammar.md
    ├── LanguageReference.md
    └── Architecture.md
```

The `examples/` directory contains 23 `.gash` files in the uploaded revision: basic demonstrations, checkpoint examples, advanced CLI examples, and `cli_test.gash`. Some examples deliberately trigger guard or lock errors; run the unified test runner to assess those correctly rather than treating every nonzero exit as an unexpected failure.

Generated `build/` directories, emitted `.cpp`/`.out` files, and platform-specific executables are **build outputs**, not compiler source files.

## 8. Diagnostics and errors

GASH distinguishes validation failures from runtime failures:

- **Lexical errors:** unrecognized tokens or malformed literals, with source positions where available.
- **Syntax errors:** malformed expressions/statements, including line and column diagnostics.
- **Semantic errors:** undeclared identifiers, incompatible types, duplicate declarations, and invalid calls.
- **C++ compilation errors:** diagnostics from the host C++ compiler if generated source cannot be compiled.
- **Runtime errors:** invalid list access, guard/lock violations, invalid restores, division/modulo by zero, invalid exponentiation, negative repeat counts, or missing required returns.

Generated programs catch runtime exceptions and report messages such as `Runtime error: Division by zero` to standard error with a nonzero exit status. Budget exhaustion has its own budget-scope handling; consult the language reference for its behaviour.

## 9. Development team

| Member | Principal responsibilities |
|---|---|
| Quynh Anh Tran | Lexer and lexical errors, CLI/CMake, lists, inspect/preview, cleanup testing, documentation |
| Gege Xin | AST, parser, grammar, checkpoints, restoration |
| Nguyen Dang Huy Pham | Semantic analysis, types, Environment, guards, locks |
| Le Bao Uyen Truong | Code generation, budgets, compiler pipeline, integration testing |

## 10. References

- [Grammar Reference](docs/Grammar.md)
- [Language Reference](docs/LanguageReference.md)
- [Compiler and Runtime Architecture](docs/Architecture.md)

The implementation in `include/`, `src/`, and `runtime/` is authoritative where documentation and behaviour differ. Test results apply to the project revision and environments in which they were obtained; they do not guarantee correctness for every possible source program.
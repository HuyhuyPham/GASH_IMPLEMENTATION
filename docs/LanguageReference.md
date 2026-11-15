# Transparent GASH — Language Reference

**COMP2001 — Language Models, Translation and Execution**

**Implementation:** C++17 Source-to-Source Compiler  

**Version:** 1.0

---

## 1. Introduction

Transparent GASH (GASH) is a statically typed programming language designed around the principle *“Nothing happens behind your back.”* It emphasizes readability, predictable execution, and explicit control over program state.

GASH combines conventional imperative programming constructs—including variables, expressions, conditional statements, loops, and functions—with transparency-oriented features for inspecting memory, controlling mutations, restoring saved state, and limiting execution.

Its principal language features include:

- **Inspectable lists:** `inspect()` displays list metadata and initialized elements.
- **State visibility:** `preview()` exposes current variable information.
- **Mutation guards:** `guard` restricts values assigned to variables.
- **Explicit mutability control:** `lock` and `unlock` control modification permissions.
- **State restoration:** `checkpoint` and `restore` save and recover selected variable states.
- **Execution budgets:** `budget` limits operations within a designated block.
- **Automatic resource management:** List-owned storage is managed through C++ object lifetimes.

GASH is implemented as a source-to-source compiler that translates `.gash` programs into C++17. Compilation consists of lexical analysis, parsing, semantic analysis, and code generation. The generated source is subsequently compiled into an executable using `g++`.

This document describes the language's syntax, type system, execution semantics, built-in functions, and runtime behaviour. Formal grammar productions are provided separately in [Grammar.md](Grammar.md).

## 2. Lexical Structure

### 2.1 General Syntax

GASH uses braces (`{}`) to delimit blocks and semicolons (`;`) to terminate statements. Whitespace and indentation improve readability but do not determine program structure.

```gash
var age = 20;

if (age >= 18) {
    output("Adult");
} otherwise {
    output("Minor");
}
```

### 2.2 Identifiers

Identifiers name variables, functions, and checkpoints. They must begin with an alphabetic character or underscore and may subsequently contain letters, digits, and underscores.

Identifiers are case-sensitive and must not conflict with reserved keywords or prohibited built-in names.

Examples of valid identifiers include `total`, `_counter`, and `item2`.

### 2.3 Literals and Data Types

GASH recognizes the following literal types:

| Type | Example | Description |
|---|---|---|
| `int` | `42` | Integer value |
| `float` | `3.14` | Floating-point value |
| `bool` | `true`, `false` | Boolean value |
| `char` | `'A'` | Character value |
| `string` | `"Hello"` | String value |

String literals use double quotation marks, while character literals use single quotation marks.

Supported escape sequences include `\n`, `\t`, `\r`, `\0`, `\\`, `\"`, and `\'`.

Malformed numeric literals, invalid escape sequences, and unterminated literals produce lexical errors.

### 2.4 Comments

GASH supports single-line and multiline comments.

Single-line comments begin with `#` and continue to the end of the line.

```gash
# Initialize the counter.
var count = 0; # Current count
```

Multiline comments are enclosed between `###` delimiters.

```gash
###
This comment spans
multiple lines.
###

var count = 10;
```

The multiline delimiters need not occupy separate lines. An unterminated multiline comment is reported as a lexical error.

### 2.5 Reserved Keywords

| Category | Keywords |
|---|---|
| Declarations | `var`, `func`, `return` |
| Conditionals | `if`, `otherwise` |
| Iteration | `while`, `repeat`, `exit`, `skip` |
| Mutation control | `guard`, `lock`, `unlock` |
| State restoration | `checkpoint`, `restore` |
| Execution control | `budget` |
| Built-in operations | `input`, `output`, `inspect`, `preview`, `extend` |
| Types | `int`, `float`, `bool`, `char`, `string`, `list` |

The words `true` and `false` are recognized as Boolean literals.

## 3. Variables and Type System

### 3.1 Variable Declaration

Variables are declared using `var` and require an initializer.

```gash
var age = 20;
var price = 15.5;
var name = "Alice";
var active = true;
```

GASH infers the variable's type from the initializer. The inferred type remains fixed throughout the variable's lifetime.

### 3.2 Static Type Checking

Assignments must preserve type compatibility.

```gash
var count = 10;
count = 20;
```

Assigning an incompatible value is invalid:

```gash
var count = 10;
count = "twenty";
```

The semantic analyzer rejects incompatible assignments before code generation.

GASH also prevents the use of undeclared variables, duplicate declarations within the same scope, and invalid assignment targets.

### 3.3 Explicit Type Conversion

GASH provides explicit conversion functions for supported primitive values.

| Function | Result type |
|---|---|
| `int(value)` | `int` |
| `float(value)` | `float` |
| `string(value)` | `string` |

Example:

```gash
var age = int("20");
var price = float("3.14");
var text = string(100);
```

Each conversion accepts exactly one argument. Invalid conversion operations are rejected through semantic or runtime validation.

### 3.4 Type Inference

Type inference occurs during semantic analysis. Variables have fixed inferred types, and function return types are determined from their return expressions.

GASH does not rely on unrestricted implicit conversions between unrelated types.

## 4. Expressions and Operators

### 4.1 Arithmetic Operators

| Operator | Operation |
|---|---|
| `+` | Addition or string concatenation |
| `-` | Subtraction |
| `*` | Multiplication |
| `/` | Division |
| `%` | Integer modulo |
| `^` | Exponentiation |

Arithmetic operations use compatible operand types. String concatenation is supported through `+` when both operands are strings.

```gash
var result = 5 + 3 * 2;
output(result);
```

Output:

```text
11
```

### 4.2 Comparison Operators

| Operator | Description |
|---|---|
| `==` | Equal |
| `!=` | Not equal |
| `<` | Less than |
| `>` | Greater than |
| `<=` | Less than or equal |
| `>=` | Greater than or equal |

Comparison expressions produce Boolean values. Operands must satisfy the applicable type restrictions.

### 4.3 Logical Operators

| Operator | Description |
|---|---|
| `&&` | Logical AND |
| `||` | Logical OR |
| `!` | Logical NOT |

Logical operators require Boolean operands.

```gash
var age = 20;
var active = true;

if (age >= 18 && active) {
    output("Allowed");
}
```

### 4.4 Operator Precedence

Operators follow the precedence order defined by the language grammar.

| Priority | Operators | Associativity |
|---|---|---|
| 1 (highest) | Parentheses | Explicit grouping |
| 2 | `^` | Right |
| 3 | Unary `-`, `!` | Unary |
| 4 | `*`, `/`, `%` | Left |
| 5 | `+`, `-` | Left |
| 6 | `<`, `>`, `<=`, `>=` | Left |
| 7 | `==`, `!=` | Left |
| 8 | `&&` | Left |
| 9 | `||` | Left |
| 10 (lowest) | `=` | Right |

Parentheses may be used to explicitly control evaluation order.

### 4.5 Assignment Expressions

Assignment updates a valid destination with a new value.

```gash
var x = 10;
x = 25;
```

Assignments are subject to static type checking and applicable runtime mutation restrictions.

## 5. Control Flow

### 5.1 Conditional Statements

GASH uses `if` and `otherwise` to perform conditional execution.

```gash
var score = 75;

if (score >= 50) {
    output("Pass");
} otherwise {
    output("Fail");
}
```

Conditions must have Boolean type. Only the selected branch executes.

### 5.2 While Loops

A `while` loop repeatedly executes its body while the condition evaluates to true.

```gash
var i = 0;

while (i < 3) {
    output(i);
    i = i + 1;
}
```

Output:

```text
0
1
2
```

The condition is evaluated before each iteration.

### 5.3 Repeat Loops

`repeat(n)` executes a block using an integer repetition count.

```gash
repeat(3) {
    output("Hello");
}
```

Output:

```text
Hello
Hello
Hello
```

The repetition count is evaluated before the loop begins. A count of zero results in no iterations.

### 5.4 Loop Control

GASH uses the keywords `exit` and `skip`.

- `exit` terminates the nearest enclosing loop.
- `skip` proceeds to the next iteration, skipping any remaining statements in the current iteration.

```gash
var i = 0;

while (i < 5) {
    i = i + 1;

    if (i == 2) {
        skip;
    }

    if (i == 4) {
        exit;
    }

    output(i);
}
```

Output:

```text
1
3
```

Using either keyword outside a loop produces a semantic error.

## 6. Functions and Scope

### 6.1 Function Declaration

Functions are declared using `func`. Parameter types are explicitly specified.

```gash
func add(int a, int b) {
    return a + b;
}

output(add(2, 3));
```

The semantic analyzer validates function declarations, parameter types, argument counts, and return expressions.

### 6.2 Return Types

Function return types are inferred through semantic analysis.

Value-returning statements must produce compatible types. Functions that do not return a value are treated as `void`.

### 6.3 Recursion

GASH supports recursive function definitions where return types can be inferred.

The semantic analyzer examines return expressions to determine the function's type and validate recursive calls.

### 6.4 Lexical Scope

GASH uses lexical block scope. Variables declared inside a block remain accessible within that scope and its permitted nested scopes.

```gash
{
    var local = 10;
    output(local);
}

output(local);
```

The final statement is invalid because `local` is outside its declared scope.

### 6.5 Variable Shadowing

Nested scopes may declare variables with names already used by outer scopes.

Each declaration represents a distinct variable in the language's scope model. References are resolved according to lexical visibility.

### 6.6 Variable Lifetime

Local variables cease to be accessible after their scope ends.

The generated program uses C++ object lifetimes and runtime Environment scope management to maintain the corresponding variable state.

## 7. Typed Lists

### 7.1 List Declaration

Lists store values of a specified element type.

```gash
var numbers = list<int>(3);
```

This creates a list with three addressable positions.

New positions are initially uninitialized.

### 7.2 Indexing

GASH uses zero-based list indexing.

```gash
var numbers = list<int>(3);

numbers[0] = 10;
numbers[1] = 20;

output(numbers[0]);
```

Output:

```text
10
```

List reads and writes are translated into runtime operations that validate indices and initialization state.

### 7.3 Size and Capacity

Lists distinguish between logical size and allocated capacity.

| Property | Description |
|---|---|
| Size | Number of addressable positions |
| Capacity | Amount of storage reserved by the underlying container |

Capacity may exceed the logical size.

### 7.4 Uninitialized Elements

Creating a list does not automatically assign values to its elements.

```gash
var numbers = list<int>(2);
output(numbers[0]);
```

The attempted read produces a runtime error because the element has not been initialized.

### 7.5 Bounds Checking

For a list with size `n`, valid indices range from `0` to `n - 1`.

Negative indices and indices beyond the valid range are rejected by the runtime.

### 7.6 Resizing

`extend()` changes a list's logical size.

```gash
var numbers = list<int>(2);

numbers[0] = 10;
numbers[1] = 20;

extend(numbers, 1);
extend(numbers, 2);
```

Shrinking a list discards elements outside the new size. Growing it again creates uninitialized positions instead of recovering previously discarded values.

The list argument must have a list type, and the new size must have integer type.

### 7.7 Copy Semantics

Lists have independent value-copy semantics.

Copying a list creates a separate collection containing copies of its stored values and initialization state.

Modifying one independent copy does not modify the other.

### 7.8 Automatic Cleanup

List storage is managed through C++ Resource Acquisition Is Initialization (RAII).

Owned resources are released automatically when the owning object is destroyed, including during normal scope exit and applicable exception unwinding.

No explicit cleanup function is required in GASH source code.

## 8. Built-in Functions

GASH provides built-in functions for input, output, conversions, and inspection.

| Function | Parameters | Description |
|---|---|---|
| `input()` | None | Read input as a string |
| `output(value)` | One value | Display a value |
| `int(value)` | One primitive value | Convert to integer |
| `float(value)` | One primitive value | Convert to floating-point |
| `string(value)` | One primitive value | Convert to string |
| `inspect(list)` | One list | Display list metadata |
| `preview()` | None | Display current variable state |
| `extend(list, size)` | List and integer | Resize a list |

### 8.1 Input and Output

`input()` reads user input and returns a string.

```gash
var name = input();
output(name);
```

Numeric input can be converted explicitly.

```gash
var age = int(input());
```

`output()` writes a value to standard output followed by a newline.

### 8.2 List Inspection

`inspect()` provides a read-only view of list storage.

```gash
var values = list<int>(2);
values[0] = 7;

inspect(values);
```

The inspection output includes the list's element type, logical size, capacity, initialized positions, and stored values.

Uninitialized positions are distinguished from initialized values.

Inspection does not modify list contents.

### 8.3 Variable Preview

`preview()` displays information about currently registered runtime variables.

```gash
var x = 10;
preview();
```

The runtime Environment supplies variable snapshots containing information such as names, types, values, lock status, and scope details.

`preview()` reports current state rather than predicting future execution.

## 9. Transparency and State Management

### 9.1 Mutation Guards

Guards restrict permitted variable values.

```gash
var balance = 100;
guard balance >= 0 && balance <= 1000;
```

A guard condition must evaluate to a Boolean value.

Mutations are validated against applicable guards before changes are committed.

```gash
var balance = 100;
guard balance >= 0;

balance = -50;
```

A guard violation rejects the invalid modification and prevents execution from proceeding normally.

The runtime Environment coordinates guarded mutations.

### 9.2 Lock and Unlock

`lock` restricts modifications to a variable, while `unlock` restores modification permission.

```gash
var balance = 100;

lock balance;
unlock balance;

balance = 200;
```

Locking and unlocking do not change a variable's static type or remove applicable guards.

Named-variable and list mutations are coordinated through runtime Environment operations.

### 9.3 Checkpoints

A checkpoint captures selected variable states using a named label.

```gash
var x = 10;
var y = 20;

checkpoint start(x, y);
```

A checkpoint must contain at least one valid variable.

The semantic analyzer validates checkpoint declarations and records type and scope information associated with the selected variables.

### 9.4 Restoration

`restore()` restores the state previously captured by a checkpoint.

```gash
var x = 10;

checkpoint start(x);

x = 100;

restore(start);

output(x);
```

Output:

```text
10
```

Checkpoint restoration is coordinated by the runtime checkpoint manager.

### 9.5 Restoration Validation

Restoring a checkpoint is subject to the language's type, scope, and mutation rules. Before changing any live variables, GASH validates the saved state against the current runtime environment.

A restoration is rejected if:

- The named checkpoint does not exist.
- A variable recorded in the checkpoint no longer exists or no longer refers to the original binding.
- The variable's scope or binding identity is incompatible with the saved snapshot.
- The variable's declared type is incompatible with the saved type.
- A target variable is locked, or an applicable guard rejects the saved value.

A variable with the same name as a previously saved variable is not necessarily the same variable. For example, declaring a replacement variable after the original binding has gone out of scope does not make the replacement a valid restoration target.

Restoration does not bypass locks or guards. If any check fails, the restore operation is rejected without applying any saved values.

### 9.6 Atomic and Repeatable Restoration

Restoration is **atomic**: all target variables must pass validation before any of them are updated. A failed restore therefore cannot leave the program in a partially restored state.

A checkpoint also remains available after a successful restore, so its saved state can be applied more than once.

```gash
var x = 10;
var y = 20;

checkpoint saved(x, y);

x = 100;
y = 200;
restore(saved);
output(x);
output(y);

x = 7;
y = 8;
restore(saved);
output(x);
output(y);
```

Expected output:

```text
10
20
10
20
```

If either `x` or `y` were locked at the time of restoration, the entire restore operation would fail; neither variable would be partially restored.

### 9.7 List Snapshots and Scope

Checkpoint snapshots of lists are independent copies of the saved list state. This includes element values, initialized and uninitialized positions, logical size, and capacity metadata. Changes made to the live list after a checkpoint do not modify the saved snapshot.

```gash
var numbers = list<int>(3);
numbers[0] = 7;
numbers[1] = 8;

checkpoint original(numbers);

numbers[0] = 99;
numbers[1] = 100;

restore(original);
output(numbers[0]);
output(numbers[1]);
```

Expected output:

```text
7
8
```

The third list position remains uninitialized because it was uninitialized when the checkpoint was created.

Checkpoint restoration also respects lexical scope and binding identity. A checkpoint cannot be used to overwrite a different variable merely because that variable has the same name as a saved one.

---

## 10. Execution Budgets

An execution budget places an explicit limit on the work performed within a block. It is written using the `budget` keyword, a parenthesized limit expression, and a braced block.

```gash
budget (10) {
    output("Hello");
}
```

The compiler translates budget statements into C++17 code that uses the GASH budget runtime. Instrumented operations in the budgeted block are counted against the applicable limit.

A budget constrains execution rather than measuring elapsed wall-clock time. When a limit is exceeded, the runtime stops normal execution of the affected budgeted block using its budget-exceeded handling mechanism. A budget does not change the static types of variables or override existing guard and lock rules.

---

## 11. Error Handling

GASH checks program correctness at multiple stages. Some errors can be detected before generating C++ code; others depend on runtime state and can only be detected during execution.

| Stage | Examples |
|---|---|
| Lexical analysis | Invalid characters, malformed literals, invalid escape sequences, or unterminated comments |
| Syntax analysis | Missing delimiters, malformed statements, or incorrectly structured expressions |
| Semantic analysis | Undeclared names, incompatible types, invalid function calls, or invalid loop-control statements |
| Runtime execution | Out-of-bounds list access, reading an uninitialized list element, guard violations, attempts to modify locked variables, or invalid checkpoint restoration |

For example, reading an uninitialized list position is syntactically valid but fails when the program executes:

```gash
var numbers = list<int>(2);
output(numbers[0]);
```

Similarly, restoring a checkpoint may be rejected at runtime if a saved variable is no longer valid or is currently locked.

The command-line compiler provides different modes for inspecting tokens, validating source code, generating C++17 code, and compiling and running a program. See the project README for the commands and their diagnostic output.

---

## 12. Related Documentation

- [README](../README.md) — Project overview, build requirements, command-line usage, and test instructions.
- [Grammar Reference](Grammar.md) — Formal syntax, production rules, and parser-related restrictions.
- [Checkpoint smoke example](../examples/checkpoint_smoke.gash) — Basic checkpoint and restore operation.
- [Multiple-variable checkpoint example](../examples/checkpoint_multi.gash) — Repeated restoration of multiple variables.
- [List checkpoint example](../examples/checkpoint_list.gash) — Restoration of independent list snapshots.

---

## 13. Summary

Transparent GASH combines conventional imperative programming constructs with explicit mechanisms for inspecting and controlling program state. Its transparency features include list inspection, variable previews, mutation guards, locks, checkpoints, restoration, and execution budgets.

The language is implemented as a C++17 source-to-source compiler. Together, its compiler stages and runtime support enable programmers to inspect state, constrain modifications, and restore previously saved values under defined language rules.
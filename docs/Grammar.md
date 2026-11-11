
# GASH Grammar Reference

**Course:** COMP2001 — Language Models, Translation and Execution

**Component:** Parser / Checkpoint Unit Tests and Grammar Reference

**Language:** GASH

**Implementation:** C++17 Recursive-Descent Parser

---

## 1. Introduction

This document describes the GASH grammar recognized by the Parser and represented in the Abstract Syntax Tree (AST).

The grammar covers:

- Variables and expressions
- Blocks and control flow
- Function declarations and calls
- Lists and indexing
- Built-in functions and type conversions
- Transparency statements, including guards, locks, checkpoints, restoration, and budgets

The Parser is responsible for checking syntax and constructing AST nodes. Semantic validation, type checking, variable scopes, and runtime behaviour are handled by other components.

This document reflects the completed parser design and the agreed team assumptions. Its lexical and syntactic rules were reviewed against `Lexer.cpp`, `Parser.cpp`, and `Token.h`. Semantic validation, code generation, and runtime behaviour are covered by separate implementation components and tests.

---

## 2. EBNF Notation

The grammar uses Extended Backus-Naur Form (EBNF).

| Symbol | Meaning |
|---|---|
| `"text"` | Exact keyword or symbol |
| `A \| B` | Either A or B |
| `[ A ]` | Optional element |
| `{ A }` | Zero or more repetitions |
| `A , B` | A followed by B |
| `;` | End of a grammar rule |

Names such as `identifier`, `integerLiteral`, and `stringLiteral` represent tokens produced by the Lexer.

Whitespace and comments are handled by the Lexer before parsing.

---

## 3. Program Structure

A GASH program contains zero or more statements.

```ebnf
program = { statement } ;

statement = block
          | varDeclaration
          | returnStatement
          | ifStatement
          | whileStatement
          | repeatStatement
          | exitStatement
          | skipStatement
          | functionDeclaration
          | guardStatement
          | lockStatement
          | unlockStatement
          | checkpointStatement
          | restoreStatement
          | budgetStatement
          | expressionStatement ;

expressionStatement = expression, ";" ;
```

An empty program is syntactically valid.

Statements may contain other statements through blocks, allowing nested control flow.

---

## 4. Variable Declarations

Variables are declared using the `var` keyword.

```ebnf
varDeclaration = "var", identifier, "=", expression, ";" ;
```

### Examples

```gash
var x = 10;
var name = "Alice";
var result = x + 5;
```

A variable declaration requires:

- A valid identifier
- An initializer expression
- A terminating semicolon

The Parser checks the declaration structure. Variable existence, type compatibility, and scope rules are checked separately.

---

## 5. Blocks

A block contains zero or more statements enclosed in braces.

```ebnf
block = "{", { statement }, "}" ;
```

### Example

```gash
{
    var x = 10;

    {
        var y = 20;
    }
}
```

Blocks may be empty or nested.

The AST represents blocks using `BlockStmt` nodes.

---

## 6. Conditional Statements

GASH supports `if` statements with an optional `otherwise` branch.

```ebnf
ifStatement = "if", "(", expression, ")", block,
              [ "otherwise", block ] ;
```

### Example

```gash
if (x > 0) {
    output(x);
} otherwise {
    output(0);
}
```

The condition must be enclosed in parentheses.

The `if` and `otherwise` bodies use braces.

The `otherwise` branch is optional.

The Parser creates an `IfStmt` node containing the condition and corresponding branches.

---

## 7. Loop Statements

### 7.1 While Loop

```ebnf
whileStatement = "while", "(", expression, ")", block ;
```

### Example

```gash
while (x < 10) {
    x = x + 1;
}
```

The condition is represented as an expression in a `WhileStmt` node.

### 7.2 Repeat Loop

```ebnf
repeatStatement = "repeat", "(", expression, ")", block ;
```

### Example

```gash
repeat (5) {
    output("Hello");
}
```

The repeat count is represented by an expression.

Validation of the count and loop execution is handled outside the Parser.

### 7.3 Loop Control

```ebnf
exitStatement = "exit", ";" ;
skipStatement = "skip", ";" ;
```

### Example

```gash
while (x < 10) {
    if (x == 5) {
        exit;
    }

    x = x + 1;
}
```

`exit` corresponds to breaking out of a loop.

`skip` corresponds to continuing to the next iteration.

The Parser recognizes these statements, while the Semantic Analyzer checks whether they appear in valid contexts.

---

## 8. Function Declarations and Returns

Functions are declared using `func`.

```ebnf
functionDeclaration = "func", identifier,
                      "(", [ parameters ], ")", block ;

parameters = parameter, { ",", parameter } ;

parameter = simpleType, identifier ;

simpleType = "int"
           | "float"
           | "string"
           | "bool"
           | "char" ;

returnStatement = "return", expression, ";" ;
```

### Example

```gash
func add(int a, int b) {
    return a + b;
}

var result = add(2, 3);
```

Function parameters require explicit types, following Assumptions A21 and A40.

The function declaration syntax does not require an explicit return type.

A `return` statement requires an expression.

Function return-type validation and legal return placement are handled by the Semantic Analyzer.

---

## 9. Transparency Statements

GASH supports transparency statements for runtime safety and state management.

These include guards, locks, checkpoints, restoration, and budgets.

### 9.1 Guard Statements

```ebnf
guardStatement = "guard", expression, ";" ;
```

### Example

```gash
guard x >= 0;
```

A guard accepts an expression without requiring additional parentheses.

The Parser creates a `GuardStmt` node.

The Runtime is responsible for enforcing guard conditions.

### 9.2 Lock and Unlock Statements

```ebnf
lockStatement = "lock", identifier, ";" ;

unlockStatement = "unlock", identifier, ";" ;
```

### Example

```gash
lock x;
unlock x;
```

The Parser recognizes the target identifier.

The Runtime determines whether a variable can be modified or unlocked.

### 9.3 Checkpoint Statements

```ebnf
checkpointStatement = "checkpoint", identifier,
                      "(", identifier,
                      { ",", identifier }, ")", ";" ;
```

### Example

```gash
checkpoint start(x, y);
```

A checkpoint requires:

- A checkpoint label
- At least one variable name
- Parentheses around the variable list
- A terminating semicolon

The Parser creates a `CheckpointStmt` node.

Checkpoint label uniqueness and variable validation are handled by the Runtime.

### 9.4 Restore Statements

```ebnf
restoreStatement = "restore",
                   "(", identifier, ")", ";" ;
```

### Example

```gash
restore(start);
```

A restore statement requires exactly one checkpoint label.

The Parser creates a `RestoreStmt` node.

The Runtime checks whether the checkpoint exists and whether restoration is permitted.

### 9.5 Budget Statements

```ebnf
budgetStatement = "budget",
                  "(", expression, ")", block ;
```

### Example

```gash
budget (10) {
    output(x);
    x = x + 1;
}
```

A budget statement contains a parenthesized expression and a block.

Budget calculation and enforcement are handled during Code Generation and Runtime execution.

---

## 10. Expression Grammar

GASH expressions are parsed using recursive descent.

Operator precedence is implemented through separate parsing methods.

The following rules are ordered from lowest to highest precedence.

```ebnf
expression = assignment ;

assignment = logicalOr, [ "=", assignment ] ;

logicalOr = logicalAnd,
            { "||", logicalAnd } ;

logicalAnd = equality,
             { "&&", equality } ;

equality = relational,
           { ("==" | "!="), relational } ;

relational = additive,
             { ("<" | ">" | "<=" | ">="), additive } ;

additive = multiplicative,
           { ("+" | "-"), multiplicative } ;

multiplicative = unary,
                 { ("*" | "/" | "%"), unary } ;

unary = ("-" | "!"), unary | power ;

power = postfix, [ "^", unary ] ;

postfix = primary, { callSuffix | indexSuffix } ;

callSuffix = "(", [ arguments ], ")" ;

indexSuffix = "[", expression, "]" ;

arguments = expression, { ",", expression } ;
```

### 10.1 Operator Precedence

| Priority | Operators | Associativity |
|---|---|---|
| Lowest | `=` | Right |
| 2 | `\|\|` | Left |
| 3 | `&&` | Left |
| 4 | `==`, `!=` | Left |
| 5 | `<`, `>`, `<=`, `>=` | Left |
| 6 | `+`, `-` | Left |
| 7 | `*`, `/`, `%` | Left |
| 8 | Unary `-`, `!` | Prefix |
| 9 | `^` | Right |
| Highest | Function calls and indexing | Postfix |

### Examples

```gash
var a = 2 + 3 * 4;
var b = (2 + 3) * 4;
var c = 2 ^ 3 ^ 2;
```

Multiplication has higher precedence than addition.

For example:

```text
2 + 3 * 4
```

is interpreted as:

```text
2 + (3 * 4)
```

Exponentiation is right-associative.

For example:

```text
2 ^ 3 ^ 2
```

is interpreted as:

```text
2 ^ (3 ^ 2)
```

### 10.2 Assignment Restrictions

The Parser only permits assignment targets represented by `VariableExpr` or `IndexExpr` AST nodes.

### Valid examples

```gash
x = 10;
numbers[0] = 5;
```

### Invalid example

```gash
(x + 1) = 10;
```

The invalid example is rejected because a binary expression is not a valid assignment target.

### 10.3 Function Call Restrictions

The EBNF describes general postfix syntax, but the Parser applies additional AST restrictions.

A function call is permitted only when its callee is represented by a `VariableExpr`.

### Valid example

```gash
add(1, 2);
```

### Invalid examples

```gash
add()(1);
numbers[0](1);
```

Calls on a previous call result or indexed value are rejected by the current Parser.

A parenthesized identifier may still be callable if it produces a `VariableExpr` AST node.

Indexing may be chained, while index bounds and type correctness are checked outside the Parser.

---

## 11. Primary Expressions and Literals

Primary expressions include literals, identifiers, function calls, list creation, and parenthesized expressions.

```ebnf
primary = literal
        | identifier
        | builtinCall
        | conversionCall
        | listCreation
        | "(", expression, ")" ;

literal = integerLiteral
        | floatLiteral
        | stringLiteral
        | charLiteral
        | booleanLiteral ;

booleanLiteral = "true" | "false" ;
```

### Examples

```gash
var count = 10;
var rate = 3.14;
var title = "GASH";
var letter = 'A';
var active = true;
var grouped = (count + 2) * 3;
```

The Lexer defines the exact syntax of identifiers, numeric literals, strings, characters, and escape sequences.

The Parser converts recognized tokens into corresponding AST expression nodes.

---

## 12. Lists and Indexing

GASH supports typed list creation and indexed access.

```ebnf
listCreation = "list", "<", simpleType,
               ">", "(", expression, ")" ;
```

List indexing uses the `indexSuffix` rule defined in Section 10.

### Example

```gash
var numbers = list<int>(3);

numbers[0] = 10;
numbers[1] = 20;
```

The Parser stores the list element type and size expression in the AST.

The Runtime manages:

- List capacity
- Element initialization
- Index bounds
- List resizing
- Deep-copy behaviour

The Parser does not perform runtime list safety checks.

---

## 13. Built-in Functions and Type Conversions

### 13.1 Built-in Functions

```ebnf
builtinCall = builtinName, callSuffix ;

builtinName = "input"
            | "output"
            | "inspect"
            | "preview"
            | "extend" ;
```

### Examples

```gash
input();
output(x);
inspect(x);
preview();
```

Built-in function names must be followed by parentheses.

A built-in name without a function call is not a valid primary expression in the current Parser.

The Parser creates `CallExpr` nodes for built-in calls.

The Runtime and Code Generator determine their actual behaviour.

### 13.2 Explicit Type Conversions

Following Assumption A40, explicit conversions use function-call syntax.

```ebnf
conversionCall = conversionName, callSuffix ;

conversionName = "int"
               | "float"
               | "string" ;
```

### Examples

```gash
var age = int(input());
var number = float("3.14");
var message = string(100);
```

Type conversions are represented using `CallExpr` nodes.

The Parser recognizes `TYPE_INT`, `TYPE_FLOAT`, and `TYPE_STRING` tokens as callable conversion names.

The Semantic Analyzer and Code Generator are responsible for validating and performing conversions.

The number and types of conversion arguments are not validated by the Parser alone.

---

## 14. Lexer Tokens and Comments

The Lexer converts GASH source code into tokens before parsing.

It recognizes:

- Keywords
- Identifiers
- Numeric, string, and character literals
- Operators
- Parentheses and braces
- Square brackets
- Commas and semicolons
- End-of-file tokens

### 14.1 Identifiers

Identifiers represent variable and function names.

Examples:

```gash
x
total
studentName
calculateSum
```

In the reviewed Lexer, identifiers begin with a letter or underscore (`_`) and continue with letters, digits, or underscores. Reserved keywords are tokenized separately and cannot be used where the Parser requires an `IDENTIFIER` token.

### 14.2 Literal Tokens

The reviewed Lexer recognizes the following forms:

- **Integers:** one or more decimal digits, such as `0` or `123`.
- **Floats:** decimal digits, a decimal point, and at least one digit after the point, such as `3.14`. A trailing decimal point (for example `12.`), exponent notation, and letters attached to a number are not supported by the current numeric scanner.
- **Strings:** double-quoted text without literal line breaks.
- **Characters:** a single character or valid escape sequence between single quotes.
- **Booleans:** the keywords `true` and `false`.

Supported string and character escapes are `\n`, `\t`, `\r`, `\0`, `\\`, `\"`, and `\'`.

### 14.3 Comments

The GASH language design specifies single-line comments:

```gash
# Single-line comment
```

And multi-line comments:

```gash
###
Multi-line comment
###
```

Both comment forms are confirmed in the reviewed `Lexer.cpp`. A `#` begins a single-line comment, while `###` begins a multi-line comment that ends at the next `###`. A `##` prefix also follows the single-line comment rule. An unterminated multi-line comment produces a lexical error.

Comments are handled by the Lexer and do not produce statement AST nodes.

---

## 15. Valid Syntax Examples

The following examples demonstrate syntactically valid GASH programs.

### 15.1 Variables and Expressions

```gash
var x = 10;
var y = 20;

var total = x + y * 2;

output(total);
```

### 15.2 Function Declaration

```gash
func multiply(int a, int b) {
    return a * b;
}

var result = multiply(3, 4);
```

### 15.3 Nested Control Flow

```gash
if (x > 0) {
    while (x < 10) {
        repeat (2) {
            output(x);
        }

        x = x + 1;
    }
}
```

### 15.4 Checkpoint and Restore

```gash
var x = 10;
var y = 20;

checkpoint start(x, y);

x = 100;
y = 200;

restore(start);
```

### 15.5 Guards and Locks

```gash
var x = 10;

guard x >= 0;

lock x;
unlock x;
```

### 15.6 Lists

```gash
var numbers = list<int>(3);

numbers[0] = 10;
numbers[1] = 20;

output(numbers[0]);
```

These examples demonstrate syntax recognized by the implemented Parser.

Successful execution also depends on semantic validation, Code Generation, and Runtime behaviour.

---

## 16. Invalid Syntax Examples

The Parser should reject the following invalid syntax.

| Invalid Syntax | Reason |
|---|---|
| `var x = ;` | Missing initializer expression |
| `var x = 10` | Missing semicolon |
| `var = 10;` | Missing variable name |
| `func add(int a,) { return a; }` | Missing parameter after comma |
| `checkpoint start();` | Missing checkpoint variable |
| `checkpoint start(x,);` | Trailing comma in checkpoint |
| `restore();` | Missing checkpoint label |
| `var x = (1 + 2;` | Missing closing parenthesis |
| `if (true) { var x = 1;` | Missing closing brace |
| `add()(1);` | Invalid function-call callee |
| `(x + 1) = 3;` | Invalid assignment target |

### Example: Missing Expression

```gash
var x = ;
```

The declaration requires an initializer expression.

### Example: Missing Closing Brace

```gash
if (true) {
    var x = 1;
```

The block is incomplete because the closing brace is missing.

### Example: Invalid Function Parameter

```gash
func add(int a,) {
    return a;
}
```

A comma must be followed by another valid parameter.

These examples illustrate syntax rejected by the implemented Parser. The registered Parser error tests passed in the final macOS test run.

---

## 17. Syntax and Semantic Responsibilities

The GASH implementation separates language processing into different components.

| Component | Responsibility |
|---|---|
| Lexer (Atq) | Converts source code into tokens |
| Parser and AST (Gloria) | Validates syntax and constructs AST nodes |
| Semantic Analyzer (Hihi) | Checks types, variable names, scopes, and legal statement contexts |
| Code Generator (Sala) | Translates AST nodes into C++17 code |
| Runtime (Hihi and G5 helper) | Handles lists, guards, locks, budgets, checkpoints, and restoration |

For example:

```gash
checkpoint start(x);
restore(start);
```

The Parser checks whether the checkpoint and restore statements follow the correct syntax.

The Semantic Analyzer and Runtime determine whether:

- Variable `x` exists.
- The checkpoint label is valid.
- The target variable has a compatible type and scope.
- The variable is locked.
- Guard conditions allow restoration.

A syntactically valid program may still fail semantic or runtime checks.

---

## 18. Checkpoint and Restore Requirements

This section records the agreed Checkpoint / Restore behaviour from Assumptions A34–A43.

These requirements are part of runtime semantics rather than the EBNF grammar.

### 18.1 Saving Checkpoints

The checkpoint system must satisfy the following requirements:

- Checkpoint labels must be unique.
- At least one variable must be specified.
- Duplicate variable names must be rejected.
- Missing variables must be rejected.
- Snapshots must preserve variable values and declared types.
- Snapshots must preserve scope and binding identities.
- Snapshots must preserve initialization states.
- List snapshots must preserve elements and capacity metadata.
- Snapshot values must be independent deep copies.
- A failed save must not create an incomplete checkpoint.

### 18.2 Restoring Checkpoints

Restoration must satisfy the following requirements:

- The checkpoint label must exist.
- All target variables must be valid.
- Scope and binding identities must remain compatible.
- Declared types must match.
- Lock and guard restrictions must be validated.
- All restoration checks must succeed before changes are committed.
- A failed restoration must leave all live variables unchanged.
- Saved checkpoints may be restored multiple times.

### 18.3 Checkpoint Interface

The `CheckpointManager` interacts with the Runtime Environment through the agreed `CheckpointEnvironment` interface.

The main methods are:

```cpp
captureVariables()
restoreVariablesAtomically()
```

`CheckpointManager` manages checkpoint labels and stored snapshots.

The actual Environment or adapter is responsible for capturing deep copies, validating restoration, and applying changes atomically.

Using `std::any` alone does not guarantee a deep copy when stored values contain shared pointers or other shared resources.

### 18.4 Runtime Integration

Following Assumptions A39 and A42:

- Generated C++ variables must be accessible through the agreed Runtime representation.
- Assignments and list writes must follow the agreed mutation interface.
- Lock and guard restrictions must not be bypassed.
- Checkpoint restoration must update the actual program state.

Checkpoint helper tests using a test Environment verify the helper contract but do not replace integration tests with the real Runtime Environment.

---

## 19. G6 Testing and Final Validation

G6 includes Parser unit tests, Checkpoint tests, and the Grammar Reference.

### 19.1 Test Files

| Test File | Purpose |
|---|---|
| `tests/test_parser.cpp` | Expression parsing and operator precedence |
| `tests/test_statement_parser.cpp` | Core statements, functions, transparency statements, and conversions |
| `tests/test_lexer_parser.cpp` | Lexer and Parser integration |
| `tests/test_parser_errors.cpp` | Invalid syntax and error handling |
| `tests/test_parser_nested.cpp` | Nested statements and boundary cases |
| `tests/test_checkpoint.cpp` | Checkpoint saving, restoration, and error handling |
| `tests/test_checkpoint_runtime.cpp` | Checkpoint integration with the real Runtime Environment |
| `tests/test_codegen_checkpoint.cpp` | Checkpoint/restore CodeGenerator integration |

### 19.2 Test Coverage

The automated test suite covers the following areas:

1. Valid GASH syntax.
2. Invalid GASH syntax.
3. Expression precedence and associativity.
4. Variable declarations and assignments.
5. Conditional statements and loops.
6. Nested blocks and control flow.
7. Function declarations and calls.
8. Type conversion expressions.
9. List creation and indexing.
10. Transparency statements.
11. Checkpoint saving and restoration.
12. Deep-copy behaviour.
13. Atomic restoration.
14. Rejection of invalid syntax and invalid checkpoint operations.

### 19.3 Final Validation

The G6 grammar review and test integration were completed for the team implementation:

- Expression and operator-precedence tests passed.
- Statement Parser tests passed, including typed parameters and conversion calls.
- Lexer-Parser integration tests passed.
- Invalid syntax and nested-statement tests passed.
- Checkpoint helper and real Runtime integration tests passed.
- Checkpoint code-generation integration tests passed.
- Lexer comment syntax was checked against the implemented `Lexer.cpp`.
- Grammar productions were compared with `Parser.cpp` and `Token.h`.
- Three checkpoint end-to-end regression tests passed using the GASH CLI.

These results were verified on macOS against the merged version after the team's latest available changes were incorporated. They do not independently establish test results for other operating systems.

### 19.4 Verified Automated Test Results

The final reported macOS Debug build completed successfully with **22/22 registered CTest tests passing** and zero failures, after incorporating the latest available team changes.

```text
100% tests passed, 0 tests failed out of 22
```

The suite includes 19 previously registered tests (including `CheckpointRuntimeTests` and `CheckpointCodegenTests`) and three automated end-to-end checkpoint tests:

| CTest Name | Example | Expected Output |
|---|---|---|
| `CheckpointSmokeE2E` | `examples/checkpoint_smoke.gash` | `10` |
| `CheckpointMultiE2E` | `examples/checkpoint_multi.gash` | `10`, `20`, `10`, `20` on separate lines |
| `CheckpointListE2E` | `examples/checkpoint_list.gash` | `7`, `8` on separate lines |

These end-to-end tests invoke the `gash` CLI, check successful execution, and compare program output against expected values. The guard and lock violation examples (`checkpoint_guard.gash` and `checkpoint_lock.gash`) are negative scenarios expected to produce runtime errors; they are not included among these three successful end-to-end CTest cases.

---

## 20. Implementation Files and Summary

### 20.1 Main Implementation Files

| File | Purpose |
|---|---|
| `include/AST.h` | AST node definitions |
| `include/Parser.h` | Parser declarations |
| `src/Parser.cpp` | Recursive-descent Parser implementation |
| `src/Lexer.cpp` | Lexer implementation |
| `runtime/Checkpoint.h` | Checkpoint interfaces and snapshot definitions |
| `runtime/Checkpoint.cpp` | Checkpoint management and restore coordination |
| `tests/` | Parser, Checkpoint, and integration tests |

### 20.2 Summary

This document provides an implementation-oriented reference for the GASH grammar.

The Parser recognizes syntactic structures and converts them into AST nodes.

The Semantic Analyzer, Code Generator, and Runtime are responsible for enforcing program correctness and execution behaviour.

The grammar covers expressions, variable declarations, control flow, functions, lists, built-in functions, type conversions, and transparency statements.

The Checkpoint / Restore requirements follow the agreed team assumptions and support the G5 and G6 testing objectives.

The completed grammar reference was checked against the implemented Lexer, Parser, and Token definitions. Following integration of the latest available team changes, all **22 registered automated tests passed on macOS**, including the three checkpoint end-to-end regression tests. The documented G5–G7 implementation and testing deliverables are complete for this revision.
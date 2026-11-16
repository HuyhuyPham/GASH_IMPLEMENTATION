# GASH Grammar Reference

**Course:** COMP2001 — Programming Languages
**Implementation:** C++17 Recursive-Descent Parser

---

## 1. Introduction

This document describes the syntax of Transparent GASH as recognized by its lexer and recursive-descent parser.

The grammar covers:

- Variable declarations and expressions
- Blocks and conditional statements
- Loops and loop-control statements
- Function declarations, including nested functions
- Typed-list creation and indexing
- Built-in functions and explicit type conversions
- Transparency statements: guards, locks, checkpoints, restoration, and execution budgets

The parser checks grammatical structure and constructs an Abstract Syntax Tree (AST). The semantic analyzer performs type checking, declaration resolution, scope validation, and other static checks.

The code generator and runtime implement execution behaviour, including guarded mutation, checkpoint restoration, arithmetic safety, and runtime diagnostics.

**Important distinction:** A syntactically valid program may still be rejected during semantic analysis or produce a runtime error.

---

## 2. EBNF Notation

The grammar uses Extended Backus–Naur Form (EBNF).

| Symbol | Meaning |
|---|---|
| `"text"` | Literal keyword or symbol |
| `A \| B` | Either A or B |
| `[ A ]` | Optional element |
| `{ A }` | Zero or more repetitions |
| `A, B` | A followed by B |
| `;` | End of an EBNF production |

Names such as `identifier`, `integerLiteral`, and `stringLiteral` represent tokens supplied by the lexer.

Whitespace and comments are processed during lexical analysis and do not appear as statements in the AST.

---

## 3. Program Structure

A GASH program consists of zero or more statements.

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

Statements can be nested inside blocks. Function declarations are also permitted inside blocks, allowing nested functions.

Whether a statement is legal in a particular context is determined by semantic analysis. For example, `return` must occur within a function, and `exit` and `skip` must occur within a loop.

---

## 4. Variable Declarations

Variables are introduced with the `var` keyword.

```ebnf
varDeclaration = "var", identifier, "=", expression, ";" ;
```

### Examples

```gash
var x = 10;
var name = "Alice";
var result = x + 5;
var enabled = true;
```

Every declaration requires a variable name, an initializer expression, and a terminating semicolon.

GASH infers the variable's static type from the initializer. Type compatibility and scope rules are enforced by semantic analysis.

A top-level variable may be accessed by functions where it is visible according to GASH scope rules. The code generator handles the corresponding C++ file-scope storage requirements.

---

## 5. Blocks and Lexical Scopes

A block is a sequence of statements enclosed in braces.

```ebnf
block = "{", { statement }, "}" ;
```

### Example

```gash
{
    var x = 10;

    {
        var y = 20;
        output(x + y);
    }
}
```

Blocks may be empty or nested.

The parser represents them using `BlockStmt` nodes.

The semantic analyzer manages lexical scopes, allowing inner scopes to access visible outer declarations and to shadow names where permitted.

The runtime Environment maintains corresponding variable-binding and scope information.

---

## 6. Conditional Statements

GASH supports `if` statements with an optional `otherwise` branch.

```ebnf
ifStatement = "if", "(", expression, ")", block,
              [ "otherwise", block ] ;
```

### Example

```gash
var x = 10;

if (x > 0) {
    output("positive");
} otherwise {
    output("non-positive");
}
```

The condition must be enclosed in parentheses.

The `if` and `otherwise` branches must use braces. An `otherwise` branch is optional.

The parser constructs an `IfStmt` node containing the condition and corresponding branches.

Semantic analysis requires the condition to have type `bool`.

---

## 7. Loop Statements

### 7.1 While Loops

```ebnf
whileStatement = "while", "(", expression, ")", block ;
```

### Example

```gash
var x = 0;

while (x < 5) {
    output(x);
    x = x + 1;
}
```

The parser produces a `WhileStmt` node.

The semantic analyzer requires the condition to have type `bool`.

### 7.2 Repeat Loops

```ebnf
repeatStatement = "repeat", "(", expression, ")", block ;
```

### Example

```gash
repeat (3) {
    output("Hello");
}
```

The repetition count must evaluate to an integer.

The parser stores the count expression in the AST, while semantic analysis validates its type.

The generated program checks the count at runtime. A negative count raises a runtime error rather than silently executing zero iterations.

### 7.3 Loop Control

```ebnf
exitStatement = "exit", ";" ;

skipStatement = "skip", ";" ;
```

### Example

```gash
var x = 0;

while (x < 10) {
    x = x + 1;

    if (x == 5) {
        skip;
    }

    if (x == 8) {
        exit;
    }

    output(x);
}
```

`exit` terminates the nearest applicable loop.

`skip` proceeds to the next iteration of the nearest applicable loop.

The parser recognizes these statements, while the semantic analyzer verifies that they occur within valid loop contexts.

---

## 8. Function Declarations and Returns

Functions are declared using `func`.

```ebnf
functionDeclaration = "func", identifier,
                      "(", [ parameters ], ")", block ;

parameters = parameter, { ",", parameter } ;

parameter = typeName, identifier ;

typeName = simpleType
         | "list", "<", simpleType, ">" ;

simpleType = "int"
           | "float"
           | "string"
           | "bool"
           | "char" ;

returnStatement = "return", expression, ";" ;
```

### 8.1 Basic Function Declaration

```gash
func add(int a, int b) {
    return a + b;
}

var result = add(2, 3);
output(result);
```

Function parameters require explicit types. Function declarations do not specify a return type; it is inferred during semantic analysis.

Functions without a value-returning statement may be inferred as returning `void`.

### 8.2 Typed-List Parameters

Functions can accept typed lists as parameters.

```gash
func first(list<int> values) {
    return values[0];
}

var numbers = list<int>(2);
numbers[0] = 9;
numbers[1] = 15;

output(first(numbers));
```

The `typeName` production permits both primitive parameter types and `list<T>` types.

List element types must be one of the supported primitive types. Nested list types are not part of the grammar.

### 8.3 Nested Function Declarations

Because `functionDeclaration` is included in `statement`, functions may be declared inside blocks and other function bodies.

```gash
func outer(int x) {
    func inner(int y) {
        return x + y;
    }

    return inner(3);
}

output(outer(5));
```

The parser produces nested `FunctionDeclStmt` nodes.

The semantic analyzer resolves function signatures, return types, and accessible variables.

The code generator implements nested functions using C++17 callable objects and lambda expressions. Recursive function calls are supported.

### 8.4 Return Statement Rules

A return statement contains an expression and must occur in a valid function context.

The semantic analyzer checks return types for consistency.

For a non-void function, reaching the end of its body without returning a value triggers a runtime error in generated code.

This is a runtime-safety rule rather than an additional EBNF restriction.

---

## 9. Transparency Statements

GASH provides syntax for runtime state observation and control.

These constructs include guards, locks, checkpoints, restoration, and execution budgets.

### 9.1 Guard Statements

```ebnf
guardStatement = "guard", expression, ";" ;
```

Example:

```gash
var balance = 100;
guard balance >= 0;
```

A guard accepts an expression without requiring additional parentheses.

Semantic analysis requires the expression to have type `bool`.

At runtime, guards validate applicable state changes. Violations are rejected, with rollback where supported.

### 9.2 Lock and Unlock

```ebnf
lockStatement = "lock", identifier, ";" ;

unlockStatement = "unlock", identifier, ";" ;
```

Example:

```gash
var x = 10;

lock x;
unlock x;

x = 20;
```

Locks prevent controlled mutations of locked variables.

For typed lists, locking also restricts element modification and resizing.

### 9.3 Checkpoint Statements

```ebnf
checkpointStatement = "checkpoint", identifier,
                      "(", identifier,
                      { ",", identifier }, ")", ";" ;
```

Example:

```gash
var x = 10;
var y = 20;

checkpoint initial(x, y);
```

A checkpoint requires a label and at least one variable identifier.

The parser constructs a `CheckpointStmt` node.

The semantic analyzer and runtime validate checkpoint labels, variable references, and scope restrictions.

### 9.4 Restore Statements

```ebnf
restoreStatement = "restore",
                   "(", identifier, ")", ";" ;
```

Example:

```gash
restore(initial);
```

A restore statement references one checkpoint label.

Restoration is subject to runtime validation, including variable identity, scope, lock, and guard restrictions.

### 9.5 Execution Budgets

```ebnf
budgetStatement = "budget",
                  "(", expression, ")", block ;
```

Example:

```gash
budget (10) {
    output("Hello");
}
```

The budget expression must have type `int`.

The code generator instruments supported operations, and the runtime tracks remaining allowances using `BudgetScope`.

Nested budgets are supported.

Budget exhaustion is handled by the runtime's budget-control mechanism.

---

## 10. Expression Grammar

GASH uses recursive-descent parsing with separate precedence levels.

The following productions are ordered from lowest to highest precedence.

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

unary = ("-" | "!"), unary
      | power ;

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
| Highest | Calls and indexing | Postfix |

### Examples

```gash
var a = 2 + 3 * 4;
var b = (2 + 3) * 4;
var c = 2 ^ 3 ^ 2;
```

Multiplication has higher precedence than addition.

Therefore:

```text
2 + 3 * 4
```

is interpreted as:

```text
2 + (3 * 4)
```

Exponentiation is right-associative:

```text
2 ^ 3 ^ 2
```

is interpreted as:

```text
2 ^ (3 ^ 2)
```

Exponentiation has higher precedence than unary negation.

Runtime arithmetic helpers validate defined invalid operations, including division by zero, modulo by zero, and invalid exponentiation results.

### 10.2 Assignment Restrictions

The parser permits assignments only when the target is represented by an assignable AST node, such as `VariableExpr` or `IndexExpr`.

Valid examples:

```gash
x = 10;
numbers[0] = 5;
```

Invalid example:

```gash
(x + 1) = 10;
```

A binary expression is not a valid assignment target.

### 10.3 Function Call Restrictions

The EBNF describes general postfix syntax, but the parser applies additional AST restrictions.

A function call requires a callee represented by `VariableExpr`.

Valid example:

```gash
add(1, 2);
```

Invalid examples:

```gash
add()(1);
numbers[0](1);
```

Calls on the result of another call or on an indexed expression are rejected by the parser.

A parenthesized identifier may remain callable if it produces a `VariableExpr`.

---

## 11. Primary Expressions and Literals

Primary expressions include literals, identifiers, built-in calls, conversions, list creation, and parenthesized expressions.

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

The lexer determines the exact forms of numeric, string, character, and Boolean literals.

The parser converts recognized literal tokens into corresponding expression nodes.

---

## 12. Typed Lists and Indexing

GASH provides typed-list creation and indexed access.

```ebnf
listCreation = "list", "<", simpleType,
               ">", "(", expression, ")" ;
```

List indexing uses the `indexSuffix` rule in Section 10.

### Example

```gash
var numbers = list<int>(3);

numbers[0] = 10;
numbers[1] = 20;

output(numbers[0]);
```

The parser stores the list element type and size expression in the AST.

Semantic analysis and runtime components enforce list type and execution rules.

These include:

- Element type compatibility
- Index bounds
- Initialized versus uninitialized positions
- List resizing
- Value-copy behaviour

Typed lists can also appear as function parameters using the `typeName` production.

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
inspect(numbers);
preview();
```

Built-in function names must be followed by parentheses.

The parser represents built-in calls using `CallExpr` nodes.

The semantic analyzer and code generator determine argument validity and execution behaviour.

`output()` supports primitive values and typed lists, including Boolean formatting as `true` and `false`.

`inspect()` displays typed-list metadata, while `preview()` displays runtime variable-state information.

### 13.2 Explicit Type Conversions

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

Conversion calls are represented as `CallExpr` nodes.

The parser recognizes conversion names as callable tokens.

Semantic analysis and runtime conversion helpers enforce argument types and conversion validity.

---

## 14. Lexer Tokens, Literals, and Comments

The lexer converts GASH source text into tokens before parsing.

Supported token categories include:

- Keywords and identifiers
- Numeric, string, character, and Boolean literals
- Arithmetic, comparison, logical, and assignment operators
- Parentheses, braces, and square brackets
- Commas and semicolons
- End-of-file tokens

### 14.1 Identifiers

Identifiers represent variable and function names.

Examples:

```text
x
total
studentName
calculateSum
```

Identifiers begin with a letter or underscore and may continue with letters, digits, or underscores.

Reserved keywords cannot be used where the grammar requires an ordinary identifier.

### 14.2 Numeric Literals

Integer literals consist of decimal digits.

Examples:

```text
0
123
1000
```

Floating-point literals contain decimal digits, a decimal point, and digits after the point.

Examples:

```text
3.14
0.5
12.0
```

The lexer does not accept forms such as a trailing decimal point (`12.`) or scientific notation under the current numeric-token rules.

### 14.3 Strings and Characters

String literals use double quotes.

Character literals use single quotes.

Examples:

```gash
var text = "Hello";
var letter = 'A';
var quote = '\'';
var nul = '\0';
```

Supported escape sequences include:

| Escape | Meaning |
|---|---|
| `\n` | Newline |
| `\t` | Tab |
| `\r` | Carriage return |
| `\0` | NUL character |
| `\\` | Backslash |
| `\"` | Double quote |
| `\'` | Single quote |

The code generator escapes characters and strings when producing C++17 literals.

### 14.4 Comments

Single-line comments start with `#`:

```gash
# This is a single-line comment
var x = 10;
```

Multiline comments use `###` delimiters:

```gash
###
This is a multiline comment.
It may span multiple lines.
###
var x = 10;
```

An unterminated multiline comment produces a lexical error.

Comments do not produce statement AST nodes.

---

## 15. Valid Syntax Examples

The following examples are syntactically valid GASH programs. Successful execution additionally requires valid types, scopes, and runtime state.

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
output(result);
```

### 15.3 Nested Function

```gash
func outer(int x) {
    func helper(int y) {
        return x + y;
    }

    return helper(5);
}

output(outer(10));
```

### 15.4 Typed-List Function Parameter

```gash
func first(list<int> values) {
    return values[0];
}

var numbers = list<int>(2);
numbers[0] = 7;
numbers[1] = 8;

output(first(numbers));
```

### 15.5 Nested Control Flow

```gash
var x = 1;

if (x > 0) {
    while (x < 4) {
        repeat (2) {
            output(x);
        }

        x = x + 1;
    }
}
```

### 15.6 Checkpoint and Restore

```gash
var x = 10;
var y = 20;

checkpoint start(x, y);

x = 100;
y = 200;

restore(start);

output(x);
output(y);
```

### 15.7 Guards and Locks

```gash
var balance = 100;

guard balance >= 0;

lock balance;
unlock balance;

balance = 75;
```

### 15.8 Typed Lists

```gash
var numbers = list<int>(3);

numbers[0] = 10;
numbers[1] = 20;

inspect(numbers);
```

---

## 16. Invalid Syntax and Semantic Examples

Not every invalid GASH program fails at the same compiler stage.

### 16.1 Invalid Syntax

The parser rejects malformed programs such as:

| Invalid example | Reason |
|---|---|
| `var x = ;` | Missing initializer expression |
| `var x = 10` | Missing semicolon |
| `var = 10;` | Missing variable name |
| `func add(int a,) { return a; }` | Missing parameter after comma |
| `checkpoint start();` | Missing checkpoint variable |
| `checkpoint start(x,);` | Trailing comma |
| `restore();` | Missing checkpoint label |
| `var x = (1 + 2;` | Missing closing parenthesis |
| `if (true) { var x = 1;` | Missing closing brace |
| `add()(1);` | Invalid function-call callee |
| `(x + 1) = 3;` | Invalid assignment target |

The parser reports syntax errors with source-location information, including line and column where available.

### 16.2 Invalid Semantic Programs

A program may be syntactically correct but semantically invalid.

For example:

```gash
output(undefinedVariable);
```

The parser can construct a valid AST for this statement, but semantic analysis rejects it because the variable has not been declared.

Other semantic errors include incompatible assignments, duplicate declarations, invalid argument counts, and incorrect statement contexts.

### 16.3 Runtime Errors

Programs can pass both syntax and semantic checks but fail during execution.

Examples include:

- Division by zero
- Modulo by zero
- Invalid exponentiation
- Negative repetition counts
- Guard violations
- Lock violations
- Invalid checkpoint restoration
- Invalid list indexing
- Reaching the end of a non-void function without returning a value

These are execution-time conditions rather than grammar errors.

---

## 17. Syntax, Semantics, and Automated Testing

The GASH implementation separates responsibilities across the compiler pipeline.

| Component | Responsibility |
|---|---|
| Lexer | Recognizes tokens and reports lexical errors |
| Parser | Validates syntax and constructs AST nodes |
| Semantic Analyzer | Checks types, declarations, scopes, function calls, and statement contexts |
| Code Generator | Produces C++17 code from validated AST constructs |
| Runtime | Enforces execution safety, lists, guards, locks, checkpoints, budgets, and state management |

### 17.1 Registered CTest Suite

Build and execute the registered tests:

```bash
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

The updated testing configuration contains **28 registered CTest tests on supported POSIX platforms**, including `AdvancedPipelineRegressionTests`.

The previously verified compiler revision passed 27/27 CTest cases. The additional C++ regression suite contains 15 cases and must be executed against the final revision to confirm the updated 28-test configuration.

### 17.2 Unified Testing Pipeline

The unified C++ test runner executes:

1. The registered CTest suite.
2. Twenty-two selected GASH CLI examples.
3. Six negative compilation tests.

Build and run it using:

```bash
c++ -std=c++17 tests/test_all.cpp -o build/test_all
./build/test_all
```

The runner verifies compilation, execution, selected expected outputs, runtime errors, and invalid-source rejection.

### 17.3 Advanced Regression Tests

The advanced C++ regression suite can be run separately:

```bash
ctest --test-dir build -R '^AdvancedPipelineRegressionTests$' --output-on-failure
```

It checks the interaction between language syntax, semantic validation, C++ generation, native compilation, and runtime behaviour for 15 targeted edge cases.

### 17.4 Related Documentation

For more information:

- [Language Reference](LanguageReference.md) — Types, language features, and execution semantics.
- [Compiler and Runtime Architecture](Architecture.md) — Compiler components, code generation, and runtime design.
- [README](../README.md) — Build instructions, CLI usage, examples, and the complete testing workflow.

The grammar describes what the parser recognizes. The Language Reference describes what valid GASH programs mean, while the Architecture document explains how those behaviours are implemented.
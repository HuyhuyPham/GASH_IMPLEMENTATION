# GASH Grammar Reference

**Course:** COMP2001 — Language Models, Translation and Execution

**Implementation:** C++17 Recursive-Descent Parser

---

## 1. Introduction

This document describes the GASH grammar recognized by the parser and represented in the Abstract Syntax Tree (AST).

The grammar covers:

- Variables and expressions
- Blocks and control flow
- Function declarations and calls
- Lists and indexing
- Built-in functions and type conversions
- Transparency statements, including guards, locks, checkpoints, restoration, and budgets

The parser is responsible for checking syntax and constructing AST nodes. Semantic validation, type checking, variable scopes, and runtime behaviour are handled by other components.

The grammar describes the syntax recognized by the lexer and parser. Semantic analysis and runtime components enforce type, scope, mutation, and execution rules.

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

Names such as `identifier`, `integerLiteral`, and `stringLiteral` represent tokens produced by the lexer.

Whitespace and comments are handled by the lexer before parsing.

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

The parser checks the declaration structure. Variable existence, type compatibility, and scope rules are checked separately.

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
var x = 1;

if (x > 0) {
    output(x);
} otherwise {
    output(0);
}
```

The condition must be enclosed in parentheses.

The `if` and `otherwise` bodies use braces.

The `otherwise` branch is optional.

The parser creates an `IfStmt` node containing the condition and corresponding branches.

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

Validation of the count and loop execution is handled outside the parser.

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

The parser recognizes these statements, while the semantic analyzer checks whether they appear in valid contexts.

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

### Example

```gash
func add(int a, int b) {
    return a + b;
}

var result = add(2, 3);
```

Function parameters require explicit types.
Function parameters require explicit types.

The function declaration syntax does not require an explicit return type.

A `return` statement requires an expression.

Function return-type validation and legal return placement are handled by the semantic analyzer.

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

The parser creates a `GuardStmt` node.

The runtime is responsible for enforcing guard conditions.

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

The parser recognizes the target identifier.

The runtime determines whether a variable can be modified or unlocked.

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

The parser creates a `CheckpointStmt` node.

The semantic analyzer validates checkpoint-label uniqueness, selected variables, and relevant scope and type requirements. The runtime manages saved snapshots.

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

The parser creates a `RestoreStmt` node.

Semantic analysis checks checkpoint references, while the runtime validates and performs restoration.

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

Budget calculation and enforcement are handled during code generation and runtime execution.

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

The parser only permits assignment targets represented by `VariableExpr` or `IndexExpr` AST nodes.

**Valid examples:**

```gash
x = 10;
numbers[0] = 5;
```

**Invalid example:**

```gash
(x + 1) = 10;
```

The invalid example is rejected because a binary expression is not a valid assignment target.

### 10.3 Function Call Restrictions

The EBNF describes general postfix syntax, but the parser applies additional AST restrictions.

A function call is permitted only when its callee is represented by a `VariableExpr`.

**Valid example:**

```gash
add(1, 2);
```

**Invalid examples:**

```gash
add()(1);
numbers[0](1);
```

Calls on a previous call result or indexed value are rejected by the current parser.

A parenthesized identifier may still be callable if it produces a `VariableExpr` AST node.

Indexing may be chained, while index bounds and type correctness are checked outside the parser.

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

The lexer defines the exact syntax of identifiers, numeric literals, strings, characters, and escape sequences.

The parser converts recognized tokens into corresponding AST expression nodes.

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

The parser stores the list element type and size expression in the AST.

The runtime manages:

- List capacity
- Element initialization
- Index bounds
- List resizing
- Deep-copy behaviour

The parser does not perform runtime list safety checks.

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

A built-in name without a function call is not a valid primary expression in the current parser.

The parser creates `CallExpr` nodes for built-in calls.

The runtime and code generator determine their actual behaviour.

### 13.2 Explicit Type Conversions

Explicit conversions use function-call syntax.
Explicit conversions use function-call syntax.

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

The parser recognizes `TYPE_INT`, `TYPE_FLOAT`, and `TYPE_STRING` tokens as callable conversion names.

The semantic analyzer and code generator are responsible for validating and performing conversions.

The number and types of conversion arguments are not validated by the parser alone.

---

## 14. Lexer Tokens and Comments

The lexer converts GASH source code into tokens before parsing.

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

Identifiers begin with a letter or underscore (`_`) and continue with letters, digits, or underscores.

Reserved keywords are tokenized separately and cannot be used where the parser requires an `IDENTIFIER` token.

### 14.2 Literal Tokens

The lexer recognizes the following forms:

- **Integers:** One or more decimal digits, such as `0` or `123`.
- **Floats:** Decimal digits, a decimal point, and at least one digit after the point, such as `3.14`.
- **Strings:** Double-quoted text without literal line breaks.
- **Characters:** A single character or valid escape sequence between single quotes.
- **Booleans:** The keywords `true` and `false`.

A trailing decimal point (for example, `12.`), exponent notation, and letters attached to a number are not accepted by the numeric scanner.

Supported string and character escapes are `\n`, `\t`, `\r`, `\0`, `\\`, `\"`, and `\'`.

### 14.3 Comments

Single-line comments:

```gash
# Single-line comment
```

Multiline comments:

```gash
###
Multiline comment
###
```

A `#` begins a single-line comment, while `###` begins a multiline comment that ends at the next `###`.

A `##` prefix also follows the single-line comment rule.

An unterminated multiline comment produces a lexical error.

Comments are handled by the lexer and do not produce statement AST nodes.

---

## 15. Valid Syntax Examples

The following examples illustrate syntactically valid GASH programs. Successful execution additionally requires valid types, scope, and runtime state.

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
var x = 1;

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

These examples demonstrate syntax recognized by the implemented parser.

Successful execution also depends on semantic validation, code generation, and runtime behaviour.

---

## 16. Invalid Syntax Examples

The parser rejects malformed syntax such as the following:

| Invalid syntax | Reason |
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

These examples illustrate syntax rejected by the implemented parser. Parser error tests cover representative invalid forms.

---

## 17. Syntax, Semantics, and Testing

The GASH implementation separates language processing into different components.

| Component | Responsibility |
|---|---|
| Lexer | Converts source text into tokens |
| Parser | Validates grammatical structure and constructs the AST |
| Semantic Analyzer | Checks declarations, types, scopes, function calls, and permitted statement contexts |
| Code Generator | Translates supported AST constructs into C++17 |
| Runtime | Handles list operations, guards, locks, checkpoints, restoration, budgets, and variable-state management |

A syntactically valid program may still be rejected by semantic analysis or fail a runtime check.

For further details, see:

- [Language Reference](LanguageReference.md) — Language features, type rules, and runtime behaviour.
- [Compiler and Runtime Architecture](Architecture.md) — Compiler pipeline, component responsibilities, and runtime implementation.

### Automated Testing

To build and run the registered tests from the repository root:

```bash
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

The latest reported macOS test run passed **22/22 tests**, including:

- `CheckpointSmokeE2E`
- `CheckpointMultiE2E`
- `CheckpointListE2E`

The test suite checks representative parser, semantic, code-generation, runtime, and end-to-end behaviour.

Full build and CLI usage instructions are provided in the [README](../README.md).
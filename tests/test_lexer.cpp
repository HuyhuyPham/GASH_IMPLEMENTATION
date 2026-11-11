#if __has_include("../include/Lexer.h")
#include "../include/Lexer.h"
#include "../include/LexicalError.h"
#include "../include/Token.h"
#else
#include "Lexer.h"
#include "LexicalError.h"
#include "Token.h"
#endif

#include <functional>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

using namespace gash;

using TT = TokenType;
using ET = LexicalErrorType;

struct Test {
    std::string id;
    std::string description;
    std::function<void()> run;
};

std::vector<Test> tests;

void require(bool condition, const std::string& message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

// Check token types, including END_OF_FILE.
void checkTokens(
    const std::string& source,
    std::vector<TT> expected
) {
    Lexer lexer(source);
    auto actual = lexer.tokenize();

    expected.push_back(TT::END_OF_FILE);

    require(actual.size() == expected.size(),
        "Wrong token count: expected " +
        std::to_string(expected.size()) +
        ", got " + std::to_string(actual.size()));

    for (std::size_t i = 0; i < expected.size(); ++i) {
        require(actual[i].type == expected[i],
            "Wrong token at index " + std::to_string(i));
    }
}

// Check expected lexical error.
void checkError(const std::string& source, ET expected) {
    try {
        Lexer lexer(source);
        lexer.tokenize();
    }
    catch (const LexicalError& error) {
        require(error.type() == expected,
            "Wrong lexical error type");
        require(error.line() >= 1 && error.column() >= 1,
            "Invalid error location");
        return;
    }

    throw std::runtime_error("Expected lexical error");
}

// Check one token's lexeme and location.
void checkPosition(
    const std::string& source,
    std::size_t index,
    const std::string& lexeme,
    std::size_t line,
    std::size_t column
) {
    Lexer lexer(source);
    auto tokens = lexer.tokenize();

    require(index < tokens.size(), "Missing token");

    const auto& t = tokens[index];

    require(t.lexeme == lexeme, "Wrong lexeme");
    require(t.line == line, "Wrong line");
    require(t.column == column, "Wrong column");
}

void valid(
    const std::string& id,
    const std::string& description,
    const std::string& source,
    std::vector<TT> expected
) {
    tests.push_back({
        id, description,
        [source, expected]() {
            checkTokens(source, expected);
        }
    });
}

void invalid(
    const std::string& id,
    const std::string& description,
    const std::string& source,
    ET error
) {
    tests.push_back({
        id, description,
        [source, error]() {
            checkError(source, error);
        }
    });
}

void position(
    const std::string& id,
    const std::string& description,
    const std::string& source,
    std::size_t index,
    const std::string& lexeme,
    std::size_t line,
    std::size_t column
) {
    tests.push_back({
        id, description,
        [=]() {
            checkPosition(source, index, lexeme, line, column);
        }
    });
}

void registerTests() {

    // L01-L12: Basic lexer functionality

    valid("L01", "Integer declaration",
        "var x = 10;",
        {TT::VAR, TT::IDENTIFIER, TT::ASSIGN,
         TT::INTEGER_LITERAL, TT::SEMICOLON});

    valid("L02", "Floating-point literal",
        "var x = 12.5;",
        {TT::VAR, TT::IDENTIFIER, TT::ASSIGN,
         TT::FLOAT_LITERAL, TT::SEMICOLON});

    valid("L03", "String literal",
        "var x = \"Hello\";",
        {TT::VAR, TT::IDENTIFIER, TT::ASSIGN,
         TT::STRING_LITERAL, TT::SEMICOLON});

    valid("L04", "Character literal",
        "var x = 'A';",
        {TT::VAR, TT::IDENTIFIER, TT::ASSIGN,
         TT::CHAR_LITERAL, TT::SEMICOLON});

    valid("L05", "Boolean literal",
        "var x = true;",
        {TT::VAR, TT::IDENTIFIER, TT::ASSIGN,
         TT::BOOL_LITERAL, TT::SEMICOLON});

    valid("L06", "Single-line comment",
        "# comment", {});

    valid("L07", "Multiline comment",
        "### comment ###", {});

    invalid("L08", "Invalid character",
        "var x = 10 @ 2;", ET::INVALID_CHARACTER);

    invalid("L09", "Unterminated string",
        "var x = \"Hello;", ET::UNTERMINATED_STRING);

    invalid("L10", "Malformed decimal number",
        "var x = 12.3.4;", ET::MALFORMED_NUMBER);

    invalid("L11", "Unterminated multiline comment",
        "### unfinished", ET::UNTERMINATED_COMMENT);

    valid("L12", "Comment symbols inside string",
        "var x = \"###\";",
        {TT::VAR, TT::IDENTIFIER, TT::ASSIGN,
         TT::STRING_LITERAL, TT::SEMICOLON});


    // L13-L25: Keywords, expressions, and grammar

    valid("L13", "Exit and skip",
        "exit; skip;",
        {TT::EXIT, TT::SEMICOLON,
         TT::SKIP, TT::SEMICOLON});

    valid("L14", "Exponentiation and modulo",
        "2 ^ 3 % 2;",
        {TT::INTEGER_LITERAL, TT::CARET,
         TT::INTEGER_LITERAL, TT::PERCENT,
         TT::INTEGER_LITERAL, TT::SEMICOLON});

    valid("L15", "Logical expression",
        "if (a != b || !c) {}",
        {TT::IF, TT::LEFT_PAREN, TT::IDENTIFIER,
         TT::BANG_EQUAL, TT::IDENTIFIER, TT::OR_OR,
         TT::BANG, TT::IDENTIFIER, TT::RIGHT_PAREN,
         TT::LEFT_BRACE, TT::RIGHT_BRACE});

    valid("L16", "List declaration",
        "var nums = list<int>(3);",
        {TT::VAR, TT::IDENTIFIER, TT::ASSIGN,
         TT::TYPE_LIST, TT::LESS, TT::TYPE_INT,
         TT::GREATER, TT::LEFT_PAREN,
         TT::INTEGER_LITERAL, TT::RIGHT_PAREN,
         TT::SEMICOLON});

    valid("L17", "Checkpoint and restore",
        "checkpoint save(x); restore(save);",
        {TT::CHECKPOINT, TT::IDENTIFIER,
         TT::LEFT_PAREN, TT::IDENTIFIER,
         TT::RIGHT_PAREN, TT::SEMICOLON,
         TT::RESTORE, TT::LEFT_PAREN,
         TT::IDENTIFIER, TT::RIGHT_PAREN,
         TT::SEMICOLON});

    valid("L18", "Escaped newline string",
        R"(var x = "Hello\nWorld";)",
        {TT::VAR, TT::IDENTIFIER, TT::ASSIGN,
         TT::STRING_LITERAL, TT::SEMICOLON});

    invalid("L19", "Invalid string escape",
        R"(var x = "Hello\q";)",
        ET::INVALID_ESCAPE_SEQUENCE);

    invalid("L20", "Overlong character literal",
        "var x = 'AB';", ET::UNTERMINATED_CHAR);

    invalid("L21", "Number followed by letters",
        "var x = 12abc;", ET::MALFORMED_NUMBER);

    invalid("L22", "Single ampersand",
        "var x = 5 & 3;", ET::INVALID_CHARACTER);

    valid("L23", "Hash symbols in string",
        R"(output("###");)",
        {TT::OUTPUT, TT::LEFT_PAREN,
         TT::STRING_LITERAL, TT::RIGHT_PAREN,
         TT::SEMICOLON});

    position("L24", "Multiline token position",
        "var a = 1;\nvar b = 2;",
        5, "var", 2, 1);

    valid("L25", "Case-sensitive keyword",
        "var Exit = 1;",
        {TT::VAR, TT::IDENTIFIER, TT::ASSIGN,
         TT::INTEGER_LITERAL, TT::SEMICOLON});


    // L26-L38: Operators and delimiters

    valid("L26", "Addition",
        "1 + 2;",
        {TT::INTEGER_LITERAL, TT::PLUS,
         TT::INTEGER_LITERAL, TT::SEMICOLON});

    valid("L27", "Subtraction",
        "1 - 2;",
        {TT::INTEGER_LITERAL, TT::MINUS,
         TT::INTEGER_LITERAL, TT::SEMICOLON});

    valid("L28", "Multiplication",
        "1 * 2;",
        {TT::INTEGER_LITERAL, TT::STAR,
         TT::INTEGER_LITERAL, TT::SEMICOLON});

    valid("L29", "Division",
        "1 / 2;",
        {TT::INTEGER_LITERAL, TT::SLASH,
         TT::INTEGER_LITERAL, TT::SEMICOLON});

    valid("L30", "Equality operator",
        "a == b;",
        {TT::IDENTIFIER, TT::EQUAL_EQUAL,
         TT::IDENTIFIER, TT::SEMICOLON});

    valid("L31", "Inequality operator",
        "a != b;",
        {TT::IDENTIFIER, TT::BANG_EQUAL,
         TT::IDENTIFIER, TT::SEMICOLON});

    valid("L32", "Less than",
        "a < b;",
        {TT::IDENTIFIER, TT::LESS,
         TT::IDENTIFIER, TT::SEMICOLON});

    valid("L33", "Less than or equal",
        "a <= b;",
        {TT::IDENTIFIER, TT::LESS_EQUAL,
         TT::IDENTIFIER, TT::SEMICOLON});

    valid("L34", "Greater than",
        "a > b;",
        {TT::IDENTIFIER, TT::GREATER,
         TT::IDENTIFIER, TT::SEMICOLON});

    valid("L35", "Greater than or equal",
        "a >= b;",
        {TT::IDENTIFIER, TT::GREATER_EQUAL,
         TT::IDENTIFIER, TT::SEMICOLON});

    valid("L36", "Logical AND",
        "a && b;",
        {TT::IDENTIFIER, TT::AND_AND,
         TT::IDENTIFIER, TT::SEMICOLON});

    valid("L37", "Logical OR",
        "a || b;",
        {TT::IDENTIFIER, TT::OR_OR,
         TT::IDENTIFIER, TT::SEMICOLON});

    valid("L38", "List indexing",
        "nums[0] = 5;",
        {TT::IDENTIFIER, TT::LEFT_BRACKET,
         TT::INTEGER_LITERAL, TT::RIGHT_BRACKET,
         TT::ASSIGN, TT::INTEGER_LITERAL,
         TT::SEMICOLON});


    // L39-L52: Errors and literal edge cases

    invalid("L39", "Invalid dollar sign",
        "$", ET::INVALID_CHARACTER);

    invalid("L40", "Invalid question mark",
        "?", ET::INVALID_CHARACTER);

    invalid("L41", "Invalid single pipe",
        "a | b", ET::INVALID_CHARACTER);

    invalid("L42", "Trailing decimal point",
        "12.", ET::MALFORMED_NUMBER);

    invalid("L43", "Two decimal points",
        "1.2.3", ET::MALFORMED_NUMBER);

    invalid("L44", "Letters after decimal",
        "1.2abc", ET::MALFORMED_NUMBER);

    invalid("L45", "Letters after integer",
        "123abc", ET::MALFORMED_NUMBER);

    invalid("L46", "Unterminated string at EOF",
        "\"hello", ET::UNTERMINATED_STRING);

    invalid("L47", "String crossing newline",
        "\"hello\nworld\"", ET::UNTERMINATED_STRING);

    invalid("L48", "Invalid backslash escape",
        R"("\z")", ET::INVALID_ESCAPE_SEQUENCE);

    invalid("L49", "Empty character literal",
        "''", ET::UNTERMINATED_CHAR);

    invalid("L50", "Unterminated character",
        "'a", ET::UNTERMINATED_CHAR);

    invalid("L51", "Invalid character escape",
        R"('\q')", ET::INVALID_ESCAPE_SEQUENCE);

    invalid("L52", "Unclosed multiline comment",
        "### hello\nworld", ET::UNTERMINATED_COMMENT);


    // L53-L58: Comments and source locations

    valid("L53", "Empty source", "", {});

    valid("L54", "Whitespace-only source",
        "   \t\n  ", {});

    valid("L55", "Comment before declaration",
        "# comment\nvar x = 1;",
        {TT::VAR, TT::IDENTIFIER, TT::ASSIGN,
         TT::INTEGER_LITERAL, TT::SEMICOLON});

    valid("L56", "Multiline comment before code",
        "### a\nb ### var x = 1;",
        {TT::VAR, TT::IDENTIFIER, TT::ASSIGN,
         TT::INTEGER_LITERAL, TT::SEMICOLON});

    position("L57", "Position after comment",
        "### comment ###\nvar x = 1;",
        0, "var", 2, 1);

    position("L58", "EOF after newline",
        "var x = 1;\n",
        5, "", 2, 1);


    // L59-L65: GASH-specific features

    valid("L59", "Lock and unlock",
        "lock x; unlock x;",
        {TT::LOCK, TT::IDENTIFIER, TT::SEMICOLON,
         TT::UNLOCK, TT::IDENTIFIER, TT::SEMICOLON});

    valid("L60", "Guard expression",
        "guard x >= 0 && x <= 100;",
        {TT::GUARD, TT::IDENTIFIER,
         TT::GREATER_EQUAL, TT::INTEGER_LITERAL,
         TT::AND_AND, TT::IDENTIFIER,
         TT::LESS_EQUAL, TT::INTEGER_LITERAL,
         TT::SEMICOLON});

    valid("L61", "Budget block",
        "budget(10) { output(x); }",
        {TT::BUDGET, TT::LEFT_PAREN,
         TT::INTEGER_LITERAL, TT::RIGHT_PAREN,
         TT::LEFT_BRACE, TT::OUTPUT,
         TT::LEFT_PAREN, TT::IDENTIFIER,
         TT::RIGHT_PAREN, TT::SEMICOLON,
         TT::RIGHT_BRACE});

    valid("L62", "Inspect and preview",
        "inspect(nums); preview();",
        {TT::INSPECT, TT::LEFT_PAREN,
         TT::IDENTIFIER, TT::RIGHT_PAREN,
         TT::SEMICOLON, TT::PREVIEW,
         TT::LEFT_PAREN, TT::RIGHT_PAREN,
         TT::SEMICOLON});

    valid("L63", "Input and conversion",
        "var age = int(input());",
        {TT::VAR, TT::IDENTIFIER, TT::ASSIGN,
         TT::TYPE_INT, TT::LEFT_PAREN,
         TT::INPUT, TT::LEFT_PAREN,
         TT::RIGHT_PAREN, TT::RIGHT_PAREN,
         TT::SEMICOLON});

    valid("L64", "Function declaration",
        "func add(int a, int b) { return a + b; }",
        {TT::FUNC, TT::IDENTIFIER,
         TT::LEFT_PAREN, TT::TYPE_INT,
         TT::IDENTIFIER, TT::COMMA,
         TT::TYPE_INT, TT::IDENTIFIER,
         TT::RIGHT_PAREN, TT::LEFT_BRACE,
         TT::RETURN, TT::IDENTIFIER,
         TT::PLUS, TT::IDENTIFIER,
         TT::SEMICOLON, TT::RIGHT_BRACE});

    valid("L65", "Extend list",
        "extend(nums, 5);",
        {TT::EXTEND, TT::LEFT_PAREN,
         TT::IDENTIFIER, TT::COMMA,
         TT::INTEGER_LITERAL, TT::RIGHT_PAREN,
         TT::SEMICOLON});
}

int main() {
    registerTests();

    int passed = 0;

    for (const auto& test : tests) {
        try {
            test.run();
            std::cout << "[PASS] "
                      << test.id << " - "
                      << test.description << '\n';
            ++passed;
        }
        catch (const std::exception& error) {
            std::cout << "[FAIL] "
                      << test.id << " - "
                      << test.description
                      << ": " << error.what() << '\n';
        }
    }

    std::cout << "\nResult: "
              << passed << "/" << tests.size()
              << " tests passed\n";

    return passed == static_cast<int>(tests.size()) ? 0 : 1;
}

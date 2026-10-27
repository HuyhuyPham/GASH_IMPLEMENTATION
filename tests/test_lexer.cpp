#include "Lexer.h"
#include "LexicalError.h"
#include "Token.h"

#include <cstddef>
#include <exception>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

using gash::Lexer;
using gash::LexicalError;
using gash::LexicalErrorType;
using gash::Token;
using gash::TokenType;

void require(bool condition, const std::string& message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

void expectTokens(const std::string& source,
                  const std::vector<Token>& expected) {
    Lexer lexer(source);
    const auto actual = lexer.tokenize();
    require(actual.size() == expected.size(),
            "Expected " + std::to_string(expected.size()) + " tokens, got " +
                std::to_string(actual.size()));

    for (std::size_t i = 0; i < expected.size(); ++i) {
        require(actual[i].type == expected[i].type,
                "Token " + std::to_string(i) + " has unexpected type");
        require(actual[i].lexeme == expected[i].lexeme,
                "Token " + std::to_string(i) + " has unexpected lexeme: '" +
                    actual[i].lexeme + "'");
        require(actual[i].line == expected[i].line &&
                    actual[i].column == expected[i].column,
                "Token " + std::to_string(i) + " has unexpected position (" +
                    std::to_string(actual[i].line) + ":" +
                    std::to_string(actual[i].column) + ")");
    }
}

void expectError(const std::string& source, LexicalErrorType expectedType) {
    try {
        Lexer lexer(source);
        (void)lexer.tokenize();
    } catch (const LexicalError& error) {
        require(error.type() == expectedType,
                "Lexer threw the wrong lexical error category");
        require(error.line() >= 1 && error.column() >= 1,
                "Lexical error must have a valid 1-based position");
        return;
    }
    throw std::runtime_error("Expected LexicalError, but tokenization succeeded");
}

struct TestCase {
    const char* id;
    const char* description;
    void (*run)();
};

void L01() {
    expectTokens("var x = 10;", {
        {TokenType::VAR, "var", 1, 1},
        {TokenType::IDENTIFIER, "x", 1, 5},
        {TokenType::ASSIGN, "=", 1, 7},
        {TokenType::INTEGER_LITERAL, "10", 1, 9},
        {TokenType::SEMICOLON, ";", 1, 11},
        {TokenType::END_OF_FILE, "", 1, 12}
    });
}

void L02() {
    expectTokens("var x = 12.5;", {
        {TokenType::VAR, "var", 1, 1},
        {TokenType::IDENTIFIER, "x", 1, 5},
        {TokenType::ASSIGN, "=", 1, 7},
        {TokenType::FLOAT_LITERAL, "12.5", 1, 9},
        {TokenType::SEMICOLON, ";", 1, 13},
        {TokenType::END_OF_FILE, "", 1, 14}
    });
}

void L03() {
    expectTokens("var x = \"Hello\";", {
        {TokenType::VAR, "var", 1, 1},
        {TokenType::IDENTIFIER, "x", 1, 5},
        {TokenType::ASSIGN, "=", 1, 7},
        {TokenType::STRING_LITERAL, "\"Hello\"", 1, 9},
        {TokenType::SEMICOLON, ";", 1, 16},
        {TokenType::END_OF_FILE, "", 1, 17}
    });
}

void L04() {
    expectTokens("var x = 'A';", {
        {TokenType::VAR, "var", 1, 1},
        {TokenType::IDENTIFIER, "x", 1, 5},
        {TokenType::ASSIGN, "=", 1, 7},
        {TokenType::CHAR_LITERAL, "'A'", 1, 9},
        {TokenType::SEMICOLON, ";", 1, 12},
        {TokenType::END_OF_FILE, "", 1, 13}
    });
}

void L05() {
    expectTokens("var x = true;", {
        {TokenType::VAR, "var", 1, 1},
        {TokenType::IDENTIFIER, "x", 1, 5},
        {TokenType::ASSIGN, "=", 1, 7},
        {TokenType::BOOL_LITERAL, "true", 1, 9},
        {TokenType::SEMICOLON, ";", 1, 13},
        {TokenType::END_OF_FILE, "", 1, 14}
    });
}

void L06() {
    expectTokens("# comment", {
        {TokenType::END_OF_FILE, "", 1, 10}
    });
}

void L07() {
    expectTokens("### comment ###", {
        {TokenType::END_OF_FILE, "", 1, 16}
    });
}

void L08() {
    expectError("var x = 10 @ 2;", LexicalErrorType::INVALID_CHARACTER);
}

void L09() {
    expectError("var x = \"Hello;", LexicalErrorType::UNTERMINATED_STRING);
}

void L10() {
    expectError("var x = 12.3.4;", LexicalErrorType::MALFORMED_NUMBER);
}

void L11() {
    expectError("### unfinished", LexicalErrorType::UNTERMINATED_COMMENT);
}

void L12() {
    expectTokens("var x = \"###\";", {
        {TokenType::VAR, "var", 1, 1},
        {TokenType::IDENTIFIER, "x", 1, 5},
        {TokenType::ASSIGN, "=", 1, 7},
        {TokenType::STRING_LITERAL, "\"###\"", 1, 9},
        {TokenType::SEMICOLON, ";", 1, 14},
        {TokenType::END_OF_FILE, "", 1, 15}
    });
}

} // namespace

int main() {
    const std::vector<TestCase> tests = {
        {"L01", "Integer tokens and positions", L01},
        {"L02", "Floating-point literal", L02},
        {"L03", "String literal", L03},
        {"L04", "Character literal", L04},
        {"L05", "Boolean literal", L05},
        {"L06", "Skip single-line comment", L06},
        {"L07", "Skip multi-line comment", L07},
        {"L08", "Invalid character", L08},
        {"L09", "Unterminated string", L09},
        {"L10", "Malformed numeric literal", L10},
        {"L11", "Unterminated multi-line comment", L11},
        {"L12", "Comment delimiters inside string", L12}
    };

    int passed = 0;
    for (const auto& test : tests) {
        try {
            test.run();
            ++passed;
            std::cout << "[PASS] " << test.id << " - " << test.description << '\n';
        } catch (const std::exception& error) {
            std::cerr << "[FAIL] " << test.id << " - " << test.description
                      << ": " << error.what() << '\n';
        }
    }

    std::cout << "\nResult: " << passed << "/" << tests.size()
              << " tests passed\n";
    return passed == static_cast<int>(tests.size()) ? 0 : 1;
}

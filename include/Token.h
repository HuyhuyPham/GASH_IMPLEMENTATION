
#ifndef GASH_TOKEN_H
#define GASH_TOKEN_H

#include <cstddef>
#include <string>
#include <string_view>

namespace gash {

enum class TokenType {
    // Identifiers and literals
    IDENTIFIER,
    INTEGER_LITERAL,
    FLOAT_LITERAL,
    STRING_LITERAL,
    CHAR_LITERAL,
    BOOL_LITERAL,

    // Declarations and control flow
    VAR,
    FUNC,
    RETURN,
    IF,
    OTHERWISE,
    WHILE,
    REPEAT,
    EXIT,
    SKIP,

    // Transparency features
    GUARD,
    LOCK,
    UNLOCK,
    CHECKPOINT,
    RESTORE,
    BUDGET,
    INSPECT,
    PREVIEW,

    // Built-in functions
    INPUT,
    OUTPUT,
    EXTEND,

    // Data types and conversions
    TYPE_INT,
    TYPE_FLOAT,
    TYPE_CHAR,
    TYPE_STRING,
    TYPE_BOOL,
    TYPE_LIST,

    // Arithmetic operators
    PLUS,             // +
    MINUS,            // -
    STAR,             // *
    SLASH,            // /
    PERCENT,          // %
    CARET,            // ^

    // Assignment and comparison
    ASSIGN,           // =
    EQUAL_EQUAL,      // ==
    BANG_EQUAL,       // !=
    LESS,             // <
    LESS_EQUAL,       // <=
    GREATER,          // >
    GREATER_EQUAL,    // >=

    // Logical operators
    AND_AND,          // &&
    OR_OR,            // ||
    BANG,             // !

    // Delimiters
    LEFT_PAREN,       // (
    RIGHT_PAREN,      // )
    LEFT_BRACE,       // {
    RIGHT_BRACE,      // }
    LEFT_BRACKET,     // [
    RIGHT_BRACKET,    // ]
    SEMICOLON,        // ;
    COMMA,            // ,

    END_OF_FILE
};

struct Token {
    TokenType type;
    std::string lexeme;
    std::size_t line;
    std::size_t column;
};

// Convert a token type into a readable name.
inline std::string_view tokenTypeName(TokenType type) {
    switch (type) {
        case TokenType::IDENTIFIER: return "IDENTIFIER";
        case TokenType::INTEGER_LITERAL: return "INTEGER_LITERAL";
        case TokenType::FLOAT_LITERAL: return "FLOAT_LITERAL";
        case TokenType::STRING_LITERAL: return "STRING_LITERAL";
        case TokenType::CHAR_LITERAL: return "CHAR_LITERAL";
        case TokenType::BOOL_LITERAL: return "BOOL_LITERAL";

        case TokenType::VAR: return "VAR";
        case TokenType::FUNC: return "FUNC";
        case TokenType::RETURN: return "RETURN";
        case TokenType::IF: return "IF";
        case TokenType::OTHERWISE: return "OTHERWISE";
        case TokenType::WHILE: return "WHILE";
        case TokenType::REPEAT: return "REPEAT";
        case TokenType::EXIT: return "EXIT";
        case TokenType::SKIP: return "SKIP";

        case TokenType::GUARD: return "GUARD";
        case TokenType::LOCK: return "LOCK";
        case TokenType::UNLOCK: return "UNLOCK";
        case TokenType::CHECKPOINT: return "CHECKPOINT";
        case TokenType::RESTORE: return "RESTORE";
        case TokenType::BUDGET: return "BUDGET";
        case TokenType::INSPECT: return "INSPECT";
        case TokenType::PREVIEW: return "PREVIEW";

        case TokenType::INPUT: return "INPUT";
        case TokenType::OUTPUT: return "OUTPUT";
        case TokenType::EXTEND: return "EXTEND";

        case TokenType::TYPE_INT: return "TYPE_INT";
        case TokenType::TYPE_FLOAT: return "TYPE_FLOAT";
        case TokenType::TYPE_CHAR: return "TYPE_CHAR";
        case TokenType::TYPE_STRING: return "TYPE_STRING";
        case TokenType::TYPE_BOOL: return "TYPE_BOOL";
        case TokenType::TYPE_LIST: return "TYPE_LIST";

        case TokenType::PLUS: return "PLUS";
        case TokenType::MINUS: return "MINUS";
        case TokenType::STAR: return "STAR";
        case TokenType::SLASH: return "SLASH";
        case TokenType::PERCENT: return "PERCENT";
        case TokenType::CARET: return "CARET";

        case TokenType::ASSIGN: return "ASSIGN";
        case TokenType::EQUAL_EQUAL: return "EQUAL_EQUAL";
        case TokenType::BANG_EQUAL: return "BANG_EQUAL";
        case TokenType::LESS: return "LESS";
        case TokenType::LESS_EQUAL: return "LESS_EQUAL";
        case TokenType::GREATER: return "GREATER";
        case TokenType::GREATER_EQUAL: return "GREATER_EQUAL";

        case TokenType::AND_AND: return "AND_AND";
        case TokenType::OR_OR: return "OR_OR";
        case TokenType::BANG: return "BANG";

        case TokenType::LEFT_PAREN: return "LEFT_PAREN";
        case TokenType::RIGHT_PAREN: return "RIGHT_PAREN";
        case TokenType::LEFT_BRACE: return "LEFT_BRACE";
        case TokenType::RIGHT_BRACE: return "RIGHT_BRACE";
        case TokenType::LEFT_BRACKET: return "LEFT_BRACKET";
        case TokenType::RIGHT_BRACKET: return "RIGHT_BRACKET";
        case TokenType::SEMICOLON: return "SEMICOLON";
        case TokenType::COMMA: return "COMMA";

        case TokenType::END_OF_FILE: return "END_OF_FILE";
    }

    return "UNKNOWN";
}

} // namespace gash

#endif // GASH_TOKEN_H

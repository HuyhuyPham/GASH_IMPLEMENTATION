// Defines LexicalErrorType and the LexicalError exception carrying source line/column diagnostics.
#ifndef GASH_LEXICAL_ERROR_H
#define GASH_LEXICAL_ERROR_H

#include <cstddef>
#include <stdexcept>
#include <string>
#include <utility>

namespace gash {

// Categorizes lexical failures detected during tokenization.
enum class LexicalErrorType {
    INVALID_CHARACTER,
    UNTERMINATED_STRING,
    UNTERMINATED_CHAR,
    INVALID_ESCAPE_SEQUENCE,
    MALFORMED_NUMBER,
    UNTERMINATED_COMMENT
};

// Exception thrown by Lexer when invalid tokens or literals are encountered.
class LexicalError : public std::runtime_error {
public:
    // Constructs a lexical error with category, diagnostic text, and 1-based location.
    LexicalError(
        LexicalErrorType type,
        std::string message,
        std::size_t line,
        std::size_t column
    )
        : std::runtime_error(std::move(message)),
          type_(type),
          line_(line),
          column_(column) {}

    // Returns the lexical error category.
    LexicalErrorType type() const noexcept {
        return type_;
    }

    // Returns the 1-based source line where the error occurred.
    std::size_t line() const noexcept {
        return line_;
    }

    // Returns the 1-based source column where the error occurred.
    std::size_t column() const noexcept {
        return column_;
    }

private:
    LexicalErrorType type_;
    std::size_t line_;
    std::size_t column_;
};

} // namespace gash

#endif // GASH_LEXICAL_ERROR_H

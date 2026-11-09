#ifndef GASH_LEXICAL_ERROR_H
#define GASH_LEXICAL_ERROR_H

#include <cstddef>
#include <stdexcept>
#include <string>
#include <utility>

namespace gash {

enum class LexicalErrorType {
    INVALID_CHARACTER,
    UNTERMINATED_STRING,
    UNTERMINATED_CHAR,
    INVALID_ESCAPE_SEQUENCE,
    MALFORMED_NUMBER,
    UNTERMINATED_COMMENT
};

class LexicalError : public std::runtime_error {
public:
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

    LexicalErrorType type() const noexcept {
        return type_;
    }

    std::size_t line() const noexcept {
        return line_;
    }

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

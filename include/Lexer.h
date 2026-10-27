
#ifndef GASH_LEXER_H
#define GASH_LEXER_H

#include <string>
#include <vector>

#include "Token.h"
#include "LexicalError.h"

namespace gash {

class Lexer {
public:
    explicit Lexer(const std::string& source);

    // Convert GASH source code into tokens.
    // Throws LexicalError if invalid source is encountered.
    std::vector<Token> tokenize();

private:
    std::string source_;

    // Current position in the source code
    std::size_t position_ = 0;

    // Source location (1-based)
    std::size_t line_ = 1;
    std::size_t column_ = 1;

    // Character navigation
    bool isAtEnd() const;
    char peek() const;
    char peekNext() const;
    char advance();
    bool match(char expected);

    // Token recognition
    void scanToken(std::vector<Token>& tokens);
    void scanIdentifier(std::vector<Token>& tokens,
                        std::size_t start,
                        std::size_t line,
                        std::size_t column);
    void scanNumber(std::vector<Token>& tokens,
                    std::size_t start,
                    std::size_t line,
                    std::size_t column);
    void scanString(std::vector<Token>& tokens,
                    std::size_t start,
                    std::size_t line,
                    std::size_t column);
    void scanChar(std::vector<Token>& tokens,
                  std::size_t start,
                  std::size_t line,
                  std::size_t column);

    // Comment handling
    void skipSingleLineComment();
    void skipMultiLineComment();

    // Token creation
    void addToken(std::vector<Token>& tokens,
                  TokenType type,
                  std::size_t start,
                  std::size_t line,
                  std::size_t column);
};

} // namespace gash

#endif // GASH_LEXER_H

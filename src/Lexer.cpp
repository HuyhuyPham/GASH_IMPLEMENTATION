#include "Lexer.h"

#include <cctype>
#include <unordered_map>

namespace gash {

namespace {

bool isAlpha(char c) {
    return std::isalpha(static_cast<unsigned char>(c)) != 0
        || c == '_';
}

bool isDigit(char c) {
    return std::isdigit(static_cast<unsigned char>(c)) != 0;
}

bool isAlphaNumeric(char c) {
    return isAlpha(c) || isDigit(c);
}

bool isValidEscape(char c) {
    return c == 'n' || c == 't' || c == 'r'
        || c == '0' || c == '\\'
        || c == '"' || c == '\'';
}

const std::unordered_map<std::string, TokenType> keywords = {
    {"var", TokenType::VAR},
    {"func", TokenType::FUNC},
    {"return", TokenType::RETURN},
    {"if", TokenType::IF},
    {"otherwise", TokenType::OTHERWISE},
    {"while", TokenType::WHILE},
    {"repeat", TokenType::REPEAT},
    {"exit", TokenType::EXIT},
    {"skip", TokenType::SKIP},

    {"guard", TokenType::GUARD},
    {"lock", TokenType::LOCK},
    {"unlock", TokenType::UNLOCK},
    {"checkpoint", TokenType::CHECKPOINT},
    {"restore", TokenType::RESTORE},
    {"budget", TokenType::BUDGET},
    {"inspect", TokenType::INSPECT},
    {"preview", TokenType::PREVIEW},

    {"input", TokenType::INPUT},
    {"output", TokenType::OUTPUT},
    {"extend", TokenType::EXTEND},

    {"int", TokenType::TYPE_INT},
    {"float", TokenType::TYPE_FLOAT},
    {"char", TokenType::TYPE_CHAR},
    {"string", TokenType::TYPE_STRING},
    {"bool", TokenType::TYPE_BOOL},
    {"list", TokenType::TYPE_LIST}
};

} // anonymous namespace

Lexer::Lexer(const std::string& source)
    : source_(source) {}

// Check whether we've consumed the entire source.
bool Lexer::isAtEnd() const {
    return position_ >= source_.size();
}

// Look at current character without consuming it.
char Lexer::peek() const {
    if (isAtEnd()) return '\0';
    return source_[position_];
}

// Look one character ahead.
char Lexer::peekNext() const {
    if (position_ + 1 >= source_.size()) return '\0';
    return source_[position_ + 1];
}

// Consume a character and update source position.
char Lexer::advance() {
    char c = source_[position_++];

    if (c == '\n') {
        ++line_;
        column_ = 1;
    } else {
        ++column_;
    }

    return c;
}

// Consume the next character only if it matches.
bool Lexer::match(char expected) {
    if (isAtEnd() || peek() != expected) {
        return false;
    }

    advance();
    return true;
}

// Add a token using its original source text.
void Lexer::addToken(
    std::vector<Token>& tokens,
    TokenType type,
    std::size_t start,
    std::size_t line,
    std::size_t column
) {
    tokens.push_back(Token{
        type,
        source_.substr(start, position_ - start),
        line,
        column
    });
}

// Main tokenization function.
std::vector<Token> Lexer::tokenize() {
    // Allow tokenize() to be called repeatedly.
    position_ = 0;
    line_ = 1;
    column_ = 1;

    std::vector<Token> tokens;

    while (!isAtEnd()) {
        scanToken(tokens);
    }

    tokens.push_back(Token{
        TokenType::END_OF_FILE,
        "",
        line_,
        column_
    });

    return tokens;
}

// Scan a keyword or identifier.
void Lexer::scanIdentifier(
    std::vector<Token>& tokens,
    std::size_t start,
    std::size_t line,
    std::size_t column
) {
    while (isAlphaNumeric(peek())) {
        advance();
    }

    std::string text = source_.substr(
        start, position_ - start
    );

    if (text == "true" || text == "false") {
        addToken(tokens, TokenType::BOOL_LITERAL,
                 start, line, column);
        return;
    }

    auto it = keywords.find(text);

    TokenType type = (it != keywords.end())
        ? it->second
        : TokenType::IDENTIFIER;

    addToken(tokens, type, start, line, column);
}

// Scan integer and floating-point literals.
void Lexer::scanNumber(
    std::vector<Token>& tokens,
    std::size_t start,
    std::size_t line,
    std::size_t column
) {
    while (isDigit(peek())) {
        advance();
    }

    bool isFloat = false;

    if (peek() == '.') {
        if (!isDigit(peekNext())) {
            throw LexicalError(
                LexicalErrorType::MALFORMED_NUMBER,
                "Malformed numeric literal",
                line, column
            );
        }

        isFloat = true;
        advance(); // decimal point

        while (isDigit(peek())) {
            advance();
        }
    }

    // Reject malformed literals like 12.3.4 or 123abc.
    if (peek() == '.' || isAlpha(peek())) {
        throw LexicalError(
            LexicalErrorType::MALFORMED_NUMBER,
            "Malformed numeric literal",
            line, column
        );
    }

    addToken(
        tokens,
        isFloat ? TokenType::FLOAT_LITERAL
                : TokenType::INTEGER_LITERAL,
        start, line, column
    );
}

// Scan a quoted string.
void Lexer::scanString(
    std::vector<Token>& tokens,
    std::size_t start,
    std::size_t line,
    std::size_t column
) {
    while (!isAtEnd() && peek() != '"') {
        if (peek() == '\n') {
            throw LexicalError(
                LexicalErrorType::UNTERMINATED_STRING,
                "Unterminated string literal",
                line, column
            );
        }

        if (peek() == '\\') {
            advance();

            if (isAtEnd() || peek() == '\n') {
                throw LexicalError(
                    LexicalErrorType::UNTERMINATED_STRING,
                    "Unterminated string literal",
                    line, column
                );
            }

            if (!isValidEscape(peek())) {
                throw LexicalError(
                    LexicalErrorType::INVALID_ESCAPE_SEQUENCE,
                    "Invalid escape sequence",
                    line_, column_
                );
            }
        }

        advance();
    }

    if (isAtEnd()) {
        throw LexicalError(
            LexicalErrorType::UNTERMINATED_STRING,
            "Unterminated string literal",
            line, column
        );
    }

    advance(); // closing quote

    addToken(tokens, TokenType::STRING_LITERAL,
             start, line, column);
}

// Scan a character literal.
void Lexer::scanChar(
    std::vector<Token>& tokens,
    std::size_t start,
    std::size_t line,
    std::size_t column
) {
    if (isAtEnd() || peek() == '\n') {
        throw LexicalError(
            LexicalErrorType::UNTERMINATED_CHAR,
            "Unterminated character literal",
            line, column
        );
    }

    if (peek() == '\'') {
        throw LexicalError(
            LexicalErrorType::UNTERMINATED_CHAR,
            "Empty character literal",
            line, column
        );
    }

    if (peek() == '\\') {
        advance();

        if (isAtEnd() || peek() == '\n') {
            throw LexicalError(
                LexicalErrorType::UNTERMINATED_CHAR,
                "Unterminated character literal",
                line, column
            );
        }

        if (!isValidEscape(peek())) {
            throw LexicalError(
                LexicalErrorType::INVALID_ESCAPE_SEQUENCE,
                "Invalid character escape sequence",
                line_, column_
            );
        }

        advance();
    } else {
        advance();
    }

    if (!match('\'')) {
        throw LexicalError(
            LexicalErrorType::UNTERMINATED_CHAR,
            "Character literal must contain one character",
            line, column
        );
    }

    addToken(tokens, TokenType::CHAR_LITERAL,
             start, line, column);
}

// Skip everything until the end of the line.
void Lexer::skipSingleLineComment() {
    while (!isAtEnd() && peek() != '\n') {
        advance();
    }
}

// Skip until a closing ### delimiter.
void Lexer::skipMultiLineComment() {
    std::size_t startLine = line_;
    std::size_t startColumn = column_ - 3;

    while (!isAtEnd()) {
        if (peek() == '#' &&
            position_ + 2 < source_.size() &&
            source_[position_ + 1] == '#' &&
            source_[position_ + 2] == '#') {

            advance();
            advance();
            advance();
            return;
        }

        advance();
    }

    throw LexicalError(
        LexicalErrorType::UNTERMINATED_COMMENT,
        "Unterminated multi-line comment",
        startLine, startColumn
    );
}

// Recognize the next token.
void Lexer::scanToken(std::vector<Token>& tokens) {
    std::size_t start = position_;
    std::size_t line = line_;
    std::size_t column = column_;

    char c = advance();

    // Whitespace
    switch (c) {
        case ' ':
        case '\t':
        case '\r':
        case '\n':
            return;
    }

    // Comments: check ### before treating # as single-line.
    if (c == '#') {
        if (peek() == '#' && peekNext() == '#') {
            advance();
            advance();
            skipMultiLineComment();
        } else {
            skipSingleLineComment();
        }
        return;
    }

    // Identifiers and numbers
    if (isAlpha(c)) {
        scanIdentifier(tokens, start, line, column);
        return;
    }

    if (isDigit(c)) {
        scanNumber(tokens, start, line, column);
        return;
    }

    switch (c) {
        // Literals
        case '"':
            scanString(tokens, start, line, column);
            return;

        case '\'':
            scanChar(tokens, start, line, column);
            return;

        // Arithmetic
        case '+':
            addToken(tokens, TokenType::PLUS,
                     start, line, column);
            return;
        case '-':
            addToken(tokens, TokenType::MINUS,
                     start, line, column);
            return;
        case '*':
            addToken(tokens, TokenType::STAR,
                     start, line, column);
            return;
        case '/':
            addToken(tokens, TokenType::SLASH,
                     start, line, column);
            return;
        case '%':
            addToken(tokens, TokenType::PERCENT,
                     start, line, column);
            return;
        case '^':
            addToken(tokens, TokenType::CARET,
                     start, line, column);
            return;

        // Assignment and comparison
        case '=':
            addToken(tokens,
                     match('=') ? TokenType::EQUAL_EQUAL
                                : TokenType::ASSIGN,
                     start, line, column);
            return;
        case '!':
            addToken(tokens,
                     match('=') ? TokenType::BANG_EQUAL
                                : TokenType::BANG,
                     start, line, column);
            return;
        case '<':
            addToken(tokens,
                     match('=') ? TokenType::LESS_EQUAL
                                : TokenType::LESS,
                     start, line, column);
            return;
        case '>':
            addToken(tokens,
                     match('=') ? TokenType::GREATER_EQUAL
                                : TokenType::GREATER,
                     start, line, column);
            return;

        // Logical operators
        case '&':
            if (!match('&')) break;
            addToken(tokens, TokenType::AND_AND,
                     start, line, column);
            return;

        case '|':
            if (!match('|')) break;
            addToken(tokens, TokenType::OR_OR,
                     start, line, column);
            return;

        // Delimiters
        case '(':
            addToken(tokens, TokenType::LEFT_PAREN,
                     start, line, column);
            return;
        case ')':
            addToken(tokens, TokenType::RIGHT_PAREN,
                     start, line, column);
            return;
        case '{':
            addToken(tokens, TokenType::LEFT_BRACE,
                     start, line, column);
            return;
        case '}':
            addToken(tokens, TokenType::RIGHT_BRACE,
                     start, line, column);
            return;
        case '[':
            addToken(tokens, TokenType::LEFT_BRACKET,
                     start, line, column);
            return;
        case ']':
            addToken(tokens, TokenType::RIGHT_BRACKET,
                     start, line, column);
            return;
        case ';':
            addToken(tokens, TokenType::SEMICOLON,
                     start, line, column);
            return;
        case ',':
            addToken(tokens, TokenType::COMMA,
                     start, line, column);
            return;
    }

    throw LexicalError(
        LexicalErrorType::INVALID_CHARACTER,
        std::string("Unexpected character '") + c + "'",
        line, column
    );
}

} // namespace gash

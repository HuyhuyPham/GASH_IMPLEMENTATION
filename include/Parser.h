
#pragma once

#include "AST.h"
#include "Token.h"

#include <cstddef>
#include <memory>
#include <vector>

namespace gash {

// Converts tokens from the Lexer into an AST.
class Parser {
public:
    // Creates a parser from a list of tokens.
    explicit Parser(std::vector<Token> tokens);

    // Parses a complete GASH program.
    Program parseProgram();

    // Parses one complete expression for testing.
    std::unique_ptr<Expr> parseExpressionOnly();

private:
    // Stores the tokens from the Lexer.
    std::vector<Token> tokens_;

    // Position of the current token.
    std::size_t current_ = 0;

    // Checks whether parsing has reached the end.
    bool isAtEnd() const;

    // Returns the current token without moving forward.
    const Token& peek() const;

    // Returns the current token and moves forward.
    const Token& advance();

    // Parses a statement.
    std::unique_ptr<Stmt> parseStatement();

    // Parses an expression.
    std::unique_ptr<Expr> parseExpression();

    // Parses assignment expressions using =.
    std::unique_ptr<Expr> parseAssignment();

    // Parses logical OR expressions using ||.
    std::unique_ptr<Expr> parseLogicalOr();

    // Parses logical AND expressions using &&.
    std::unique_ptr<Expr> parseLogicalAnd();

    // Parses equality operators == and !=.
    std::unique_ptr<Expr> parseEquality();

    // Parses comparison operators such as <, >, <= and >=.
    std::unique_ptr<Expr> parseRelational();

    // Parses addition and subtraction.
    std::unique_ptr<Expr> parseAdditive();

    // Parses multiplication, division and modulo.
    std::unique_ptr<Expr> parseMultiplicative();

    // Parses unary operators such as - and !.
    std::unique_ptr<Expr> parseUnary();

    // Parses exponentiation using the ^ operator.
    std::unique_ptr<Expr> parsePower();

    // Parses function calls and list indexing.
    std::unique_ptr<Expr> parsePostfix();

    // Parses basic expressions such as numbers and variables.
    std::unique_ptr<Expr> parsePrimary();
};

} // namespace gash
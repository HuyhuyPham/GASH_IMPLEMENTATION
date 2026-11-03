
#include "Parser.h"

#include <stdexcept>
#include <utility>

namespace gash {

// Stores the token list provided by the Lexer.
Parser::Parser(std::vector<Token> tokens)
    : tokens_(std::move(tokens)) {

    // Ensure the token list ends with EOF.
    if (tokens_.empty() ||
        tokens_.back().type != TokenType::END_OF_FILE) {

        tokens_.push_back(
            Token{TokenType::END_OF_FILE, "", 1, 1}
        );
    }
}

// Checks whether we have reached the EOF token.
bool Parser::isAtEnd() const {
    return peek().type == TokenType::END_OF_FILE;
}

// Returns the current token without moving forward.
const Token& Parser::peek() const {
    return tokens_[current_];
}

// Returns the current token and moves forward.
const Token& Parser::advance() {
    const Token& token = peek();

    if (!isAtEnd()) {
        current_++;
    }

    return token;
}

// Parses an expression.
std::unique_ptr<Expr> Parser::parseExpression() {

    // Start with assignment expressions.
    return parseAssignment();
}

// Parses one complete expression for testing.
std::unique_ptr<Expr> Parser::parseExpressionOnly() {

    // Parse the expression.
    auto expression = parseExpression();

    // Check that no unexpected tokens remain.
    if (!isAtEnd()) {
        throw std::runtime_error(
            "Unexpected token after expression: " + peek().lexeme
        );
    }

    // Return the completed AST.
    return expression;
}

// Parses assignment expressions using =.
std::unique_ptr<Expr> Parser::parseAssignment() {

    // Parse the left expression.
    auto left = parseLogicalOr();

    // Check whether the next token is =.
    if (peek().type == TokenType::ASSIGN) {

        // Read and consume the assignment operator.
        advance();

        // Check whether the left side can be assigned to.
        if (dynamic_cast<VariableExpr*>(left.get()) == nullptr &&
            dynamic_cast<IndexExpr*>(left.get()) == nullptr) {

            throw std::runtime_error("Invalid assignment target");
        }

        // Parse the right expression recursively.
        auto right = parseAssignment();

        // Create and return the assignment AST node.
        return std::make_unique<AssignmentExpr>(
            std::move(left),
            std::move(right)
        );
    }

    // Return the expression if there is no assignment.
    return left;
}

// Parses logical OR expressions using ||.
std::unique_ptr<Expr> Parser::parseLogicalOr() {

    // Parse the left expression.
    auto left = parseLogicalAnd();

    // Continue while the next token is ||.
    while (peek().type == TokenType::OR_OR) {

        // Read and consume the || operator.
        std::string op = advance().lexeme;

        // Parse the right expression.
        auto right = parseLogicalAnd();

        // Create a binary expression.
        left = std::make_unique<BinaryExpr>(
            op,
            std::move(left),
            std::move(right)
        );
    }

    // Return the completed expression.
    return left;
}


// Parses logical AND expressions using &&.
std::unique_ptr<Expr> Parser::parseLogicalAnd() {

    // Parse the left expression.
    auto left = parseEquality();

    // Continue while the next token is &&.
    while (peek().type == TokenType::AND_AND) {

        // Read and consume the && operator.
        std::string op = advance().lexeme;

        // Parse the right expression.
        auto right = parseEquality();

        // Create a binary expression.
        left = std::make_unique<BinaryExpr>(
            op,
            std::move(left),
            std::move(right)
        );
    }

    // Return the completed expression.
    return left;
}

// Parses equality operators == and !=.
std::unique_ptr<Expr> Parser::parseEquality() {

    // Parse the left expression.
    auto left = parseRelational();

    // Continue while the next token is == or !=.
    while (peek().type == TokenType::EQUAL_EQUAL ||
           peek().type == TokenType::BANG_EQUAL) {

        // Read and consume the operator.
        std::string op = advance().lexeme;

        // Parse the right expression.
        auto right = parseRelational();

        // Create a binary expression.
        left = std::make_unique<BinaryExpr>(
            op,
            std::move(left),
            std::move(right)
        );
    }

    // Return the completed expression.
    return left;
}

// Parses comparison operators such as <, >, <= and >=.
std::unique_ptr<Expr> Parser::parseRelational() {

    // Parse the left expression.
    auto left = parseAdditive();

    // Continue while the next token is a comparison operator.
    while (peek().type == TokenType::LESS ||
           peek().type == TokenType::GREATER ||
           peek().type == TokenType::LESS_EQUAL ||
           peek().type == TokenType::GREATER_EQUAL) {

        // Read the operator.
        std::string op = advance().lexeme;

        // Parse the right expression.
        auto right = parseAdditive();

        // Create a binary expression.
        left = std::make_unique<BinaryExpr>(
            op,
            std::move(left),
            std::move(right)
        );
    }

    // Return the completed expression.
    return left;
}

// Parses multiplication, division and modulo.
std::unique_ptr<Expr> Parser::parseMultiplicative() {

    // Parse the first expression.
    auto left = parseUnary();

    // Continue while the next token is *, / or %.
    while (peek().type == TokenType::STAR ||
           peek().type == TokenType::SLASH ||
           peek().type == TokenType::PERCENT) {

        // Read the operator.
        std::string op = advance().lexeme;

        // Parse the right expression.
        auto right = parseUnary();

        // Create a binary expression.
        left = std::make_unique<BinaryExpr>(
            op,
            std::move(left),
            std::move(right)
        );
    }

    // Return the completed expression.
    return left;
}

// Parses addition and subtraction.
std::unique_ptr<Expr> Parser::parseAdditive() {

    // Parse the left expression.
    auto left = parseMultiplicative();

    // Continue while the next token is + or -.
    while (peek().type == TokenType::PLUS ||
           peek().type == TokenType::MINUS) {

        // Read the operator.
        std::string op = advance().lexeme;

        // Parse the right expression.
        auto right = parseMultiplicative();

        // Create a binary expression.
        left = std::make_unique<BinaryExpr>(
            op,
            std::move(left),
            std::move(right)
        );
    }

    // Return the completed expression.
    return left;
}

// Parses unary operators such as - and !.
std::unique_ptr<Expr> Parser::parseUnary() {

    // Check whether the current token is - or !.
    if (peek().type == TokenType::MINUS ||
        peek().type == TokenType::BANG) {

        // Read and consume the unary operator.
        std::string op = advance().lexeme;

        // Parse the expression after the operator.
        auto operand = parseUnary();

        // Create and return the unary AST node.
        return std::make_unique<UnaryExpr>(
            op,
            std::move(operand)
        );
    }

    // Otherwise, parse exponentiation.
    return parsePower();
}

// Parses exponentiation using ^.
std::unique_ptr<Expr> Parser::parsePower() {

    // Parse the left expression.
    auto left = parsePostfix();

    // Check whether the next token is ^.
    if (peek().type == TokenType::CARET) {

        // Read and consume the ^ operator.
        std::string op = advance().lexeme;

        // Parse the right expression.
        auto right = parseUnary();

        // Create and return the power AST node.
        return std::make_unique<BinaryExpr>(
            op,
            std::move(left),
            std::move(right)
        );
    }

    // Return the expression if there is no ^.
    return left;
}

// Parses function calls and list indexing.
std::unique_ptr<Expr> Parser::parsePostfix() {

    // Parse the first basic expression.
    auto expression = parsePrimary();

    // Continue checking for function calls or indexing.
    while (true) {

        // Check for a function call.
        if (peek().type == TokenType::LEFT_PAREN) {

            // Make sure the expression is a function name.
            auto* variable =
                dynamic_cast<VariableExpr*>(expression.get());

            if (variable == nullptr) {
                throw std::runtime_error("Invalid function call");
            }

            // Save the function name.
            std::string name = variable->name;

            // Consume '('.
            advance();

            // Store the function arguments.
            std::vector<std::unique_ptr<Expr>> arguments;

            // Check whether there are any arguments.
            if (peek().type != TokenType::RIGHT_PAREN) {

                while (true) {

                    // Parse one argument.
                    arguments.push_back(parseExpression());

                    // Stop if there is no comma.
                    if (peek().type != TokenType::COMMA) {
                        break;
                    }

                    // Consume ','.
                    advance();
                }
            }

            // Check for ')'.
            if (peek().type != TokenType::RIGHT_PAREN) {
                throw std::runtime_error("Expected ')' after arguments");
            }

            // Consume ')'.
            advance();

            // Create the function call AST node.
            expression = std::make_unique<CallExpr>(
                std::move(name),
                std::move(arguments)
            );

            continue;
        }

        // Check for list indexing.
        if (peek().type == TokenType::LEFT_BRACKET) {

            // Consume '['.
            advance();

            // Parse the index expression.
            auto index = parseExpression();

            // Check for ']'.
            if (peek().type != TokenType::RIGHT_BRACKET) {
                throw std::runtime_error("Expected ']'");
            }

            // Consume ']'.
            advance();

            // Create the list indexing AST node.
            expression = std::make_unique<IndexExpr>(
                std::move(expression),
                std::move(index)
            );

            continue;
        }

        // Stop if there are no more postfix operations.
        break;
    }

    // Return the completed expression.
    return expression;
}

// Parses a basic expression.
std::unique_ptr<Expr> Parser::parsePrimary() {

    // Check whether the current token is an integer.
    if (peek().type == TokenType::INTEGER_LITERAL) {

        // Read and consume the integer token.
        const Token& token = advance();

        // Convert the text into an integer.
        int value = std::stoi(token.lexeme);

        // Create and return the integer AST node.
        return std::make_unique<IntegerExpr>(value);
    }

    // Check whether the current token is a floating-point number.
    if (peek().type == TokenType::FLOAT_LITERAL) {

        // Read and consume the float token.
        const Token& token = advance();

        // Convert the text into a double.
        double value = std::stod(token.lexeme);

        // Create and return the float AST node.
        return std::make_unique<FloatExpr>(value);
    }

    // Check whether the current token is a string.
    if (peek().type == TokenType::STRING_LITERAL) {

        // Read and consume the string token.
        const Token& token = advance();

        // Get the text from the token.
        std::string value = token.lexeme;

        // Remove surrounding double quotes if present.
        if (value.size() >= 2 &&
            value.front() == '"' &&
            value.back() == '"') {

            value = value.substr(1, value.size() - 2);
        }

        // Create and return the string AST node.
        return std::make_unique<StringExpr>(std::move(value));
    }

    // Check whether the current token is a Boolean literal.
    if (peek().type == TokenType::BOOL_LITERAL) {

        // Read and consume the Boolean token.
        const Token& token = advance();

        // Convert the text into a Boolean value.
        bool value = (token.lexeme == "true");

        // Create and return the Boolean AST node.
        return std::make_unique<BoolExpr>(value);
    }

    // Check whether the current token is a character literal.
    if (peek().type == TokenType::CHAR_LITERAL) {

        // Read and consume the character token.
        const Token& token = advance();

        // Check the character literal format.
        if (token.lexeme.size() != 3 ||
            token.lexeme.front() != '\'' ||
            token.lexeme.back() != '\'') {

            throw std::runtime_error("Invalid character literal");
        }

        // Get the character between the single quotes.
        char value = token.lexeme[1];

        // Create and return the character AST node.
        return std::make_unique<CharExpr>(value);
    }

    
    // Check for an identifier or built-in function name.
    if (peek().type == TokenType::IDENTIFIER ||
        peek().type == TokenType::INSPECT ||
        peek().type == TokenType::PREVIEW ||
        peek().type == TokenType::INPUT ||
        peek().type == TokenType::OUTPUT ||
        peek().type == TokenType::EXTEND) {

        // Check whether the token is a built-in function.
        bool isBuiltin =
            peek().type != TokenType::IDENTIFIER;

        // Read and consume the token.
        const Token& token = advance();

        // Built-in functions must be followed by '('.
        if (isBuiltin &&
            peek().type != TokenType::LEFT_PAREN) {
            throw std::runtime_error(
                "Expected '(' after built-in function"
            );
        }

        // Return the name for function call parsing.
        return std::make_unique<VariableExpr>(token.lexeme);
    }

    // Check whether the current token is a list constructor.
    if (peek().type == TokenType::TYPE_LIST) {

        // Consume 'list'.
        advance();

        // Check for '<'.
        if (peek().type != TokenType::LESS) {
            throw std::runtime_error("Expected '<' after list");
        }

        // Consume '<'.
        advance();

        // Store the list element type.
        std::string elementType;

        // Determine the element type.
        switch (peek().type) {
            case TokenType::TYPE_INT:
                elementType = "int";
                break;

            case TokenType::TYPE_FLOAT:
                elementType = "float";
                break;

            case TokenType::TYPE_CHAR:
                elementType = "char";
                break;

            case TokenType::TYPE_STRING:
                elementType = "string";
                break;

            case TokenType::TYPE_BOOL:
                elementType = "bool";
                break;

            default:
                throw std::runtime_error(
                    "Expected a valid list element type"
                );
        }

        // Consume the element type.
        advance();

        // Check for '>'.
        if (peek().type != TokenType::GREATER) {
            throw std::runtime_error("Expected '>' after list type");
        }

        // Consume '>'.
        advance();

        // Check for '('.
        if (peek().type != TokenType::LEFT_PAREN) {
            throw std::runtime_error("Expected '(' after list type");
        }

        // Consume '('.
        advance();

        // Parse the list size.
        auto size = parseExpression();

        // Check for ')'.
        if (peek().type != TokenType::RIGHT_PAREN) {
            throw std::runtime_error("Expected ')' after list size");
        }

        // Consume ')'.
        advance();

        // Create and return the list AST node.
        return std::make_unique<ListExpr>(
            std::move(elementType),
            std::move(size)
        );
    }

    // Check whether the current token is '('.
    if (peek().type == TokenType::LEFT_PAREN) {

        // Consume the opening parenthesis.
        advance();

        // Parse the expression inside the parentheses.
        auto expression = parseExpression();

        // Check for the closing parenthesis.
        if (peek().type != TokenType::RIGHT_PAREN) {
            throw std::runtime_error("Expected ')'");
        }

        // Consume the closing parenthesis.
        advance();

        // Return the expression inside the parentheses.
        return expression;
    }

    // Report an error for unsupported tokens.
    throw std::runtime_error("Expected an expression");
}


} // namespace gash
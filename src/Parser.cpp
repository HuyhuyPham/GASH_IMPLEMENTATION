
#include "Parser.h"

#include <stdexcept>
#include <utility>

namespace gash {

namespace {

// Converts an escape sequence into its character value.
char decodeEscape(char c) {
    switch (c) {
        case 'n':  return '\n';
        case 't':  return '\t';
        case 'r':  return '\r';
        case '0':  return '\0';
        case '\\': return '\\';
        case '"':  return '"';
        case '\'': return '\'';

        default:
            throw std::runtime_error("Invalid escape sequence");
    }
}

// Creates an AST node and records its source location.
template <typename T, typename... Args>
std::unique_ptr<T> makeLocated(
    const Token& token,
    Args&&... args
) {
    // Create the AST node.
    auto node = std::make_unique<T>(
        std::forward<Args>(args)...
    );

    // Record the token's source position.
    node->location = {
        static_cast<int>(token.line),
        static_cast<int>(token.column)
    };

    // Return the completed node.
    return node;
}

} // anonymous namespace

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

// Parses a complete GASH program.
Program Parser::parseProgram() {

    // Create an empty program.
    Program program;

    // Keep parsing until the end of the file.
    while (!isAtEnd()) {

        // Parse one statement and add it to the program.
        program.statements.push_back(parseStatement());
    }

    // Return the completed program.
    return program;
}



// Parses a statement.
std::unique_ptr<Stmt> Parser::parseStatement() {

    // Check for a block statement.
    if (peek().type == TokenType::LEFT_BRACE) {
        return parseBlock();
    }

    // Check for a variable declaration.
    if (peek().type == TokenType::VAR) {
        return parseVarDeclaration();
    }

    // Check for a return statement.
    if (peek().type == TokenType::RETURN) {
        return parseReturnStatement();
    }

    // Check for an if statement.
    if (peek().type == TokenType::IF) {
        return parseIfStatement();
    }

    // Check for a while statement.
    if (peek().type == TokenType::WHILE) {
        return parseWhileStatement();
    }

    // Check for a repeat statement.
    if (peek().type == TokenType::REPEAT) {
        return parseRepeatStatement();
    }

    // Check for an exit statement.
    if (peek().type == TokenType::EXIT) {
        return parseExitStatement();
    }

    // Check for a skip statement.
    if (peek().type == TokenType::SKIP) {
        return parseSkipStatement();
    }

    // Check for a function declaration.
    if (peek().type == TokenType::FUNC) {
        return parseFunctionDeclaration();
    }

    // Check for a guard statement.
    if (peek().type == TokenType::GUARD) {
        return parseGuardStatement();
    }

    // Check for a lock statement.
    if (peek().type == TokenType::LOCK) {
        return parseLockStatement();
    }

    // Check for an unlock statement.
    if (peek().type == TokenType::UNLOCK) {
        return parseUnlockStatement();
    }

    // Check for a checkpoint statement.
    if (peek().type == TokenType::CHECKPOINT) {
        return parseCheckpointStatement();
    }

    // Check for a restore statement.
    if (peek().type == TokenType::RESTORE) {
        return parseRestoreStatement();
    }

    // Check for a budget statement.
    if (peek().type == TokenType::BUDGET) {
        return parseBudgetStatement();
    }

    // Save the statement's starting token.
    const Token& startToken = peek();

    // Parse an expression.
    auto expression = parseExpression();

    // Check for the semicolon.
    if (peek().type != TokenType::SEMICOLON) {
        throw std::runtime_error(
            "Expected ';' after expression"
        );
    }

    // Consume the semicolon.
    advance();

    // Create and return the expression statement.
    return makeLocated<ExpressionStmt>(
        startToken,
        std::move(expression)
    );
}

// Parses statements inside { }.
std::unique_ptr<BlockStmt> Parser::parseBlock() {

    // Read and consume the opening brace.
    const Token& openBrace = advance();

    // Store the statements inside the block.
    std::vector<std::unique_ptr<Stmt>> statements;

    // Continue until '}' or end of file.
    while (peek().type != TokenType::RIGHT_BRACE &&
           !isAtEnd()) {

        // Parse one statement.
        statements.push_back(parseStatement());
    }

    // Check for the closing brace.
    if (peek().type != TokenType::RIGHT_BRACE) {
        throw std::runtime_error(
            "Expected '}' after block"
        );
    }

    // Consume the closing brace.
    advance();

    // Create and return the block AST node.
    return makeLocated<BlockStmt>(
        openBrace,
        std::move(statements)
    );
}

// Parses variable declarations using var.
std::unique_ptr<Stmt> Parser::parseVarDeclaration() {

    // Read and consume the 'var' keyword.
    const Token& varToken = advance();

    // Check for a variable name.
    if (peek().type != TokenType::IDENTIFIER) {
        throw std::runtime_error(
            "Expected variable name after 'var'"
        );
    }

    // Read and consume the variable name.
    std::string name = advance().lexeme;

    // Check for the assignment operator.
    if (peek().type != TokenType::ASSIGN) {
        throw std::runtime_error(
            "Expected '=' after variable name"
        );
    }

    // Consume '='.
    advance();

    // Parse the initial value.
    auto initializer = parseExpression();

    // Check for the semicolon.
    if (peek().type != TokenType::SEMICOLON) {
        throw std::runtime_error(
            "Expected ';' after variable declaration"
        );
    }

    // Consume the semicolon.
    advance();

    // Create and return the variable declaration AST node.
    return makeLocated<VarDeclStmt>(
        varToken,
        std::move(name),
        std::move(initializer)
    );
}

// Parses return statements.
std::unique_ptr<Stmt> Parser::parseReturnStatement() {

    // Read and consume the 'return' keyword.
    const Token& returnToken = advance();

    // Parse the return value.
    auto value = parseExpression();

    // Check for the semicolon.
    if (peek().type != TokenType::SEMICOLON) {
        throw std::runtime_error(
            "Expected ';' after return statement"
        );
    }

    // Consume the semicolon.
    advance();

    // Create and return the ReturnStmt AST node.
    return makeLocated<ReturnStmt>(
        returnToken,
        std::move(value)
    );
}

// Parses if and otherwise statements.
std::unique_ptr<Stmt> Parser::parseIfStatement() {

    // Read and consume the 'if' keyword.
    const Token& ifToken = advance();

    // Check for '('.
    if (peek().type != TokenType::LEFT_PAREN) {
        throw std::runtime_error("Expected '(' after if");
    }

    // Consume '('.
    advance();

    // Parse the condition.
    auto condition = parseExpression();

    // Check for ')'.
    if (peek().type != TokenType::RIGHT_PAREN) {
        throw std::runtime_error("Expected ')' after if condition");
    }

    // Consume ')'.
    advance();

    // Check for the opening brace.
    if (peek().type != TokenType::LEFT_BRACE) {
        throw std::runtime_error("Expected '{' after if condition");
    }

    // Parse the if block.
    auto thenBranch = parseBlock();

    // Store the optional otherwise block.
    std::unique_ptr<BlockStmt> elseBranch = nullptr;

    // Check for 'otherwise'.
    if (peek().type == TokenType::OTHERWISE) {

        // Consume 'otherwise'.
        advance();

        // Check for the opening brace.
        if (peek().type != TokenType::LEFT_BRACE) {
            throw std::runtime_error("Expected '{' after otherwise");
        }

        // Parse the otherwise block.
        elseBranch = parseBlock();
    }

    // Create and return the IfStmt AST node.
    return makeLocated<IfStmt>(
        ifToken,
        std::move(condition),
        std::move(thenBranch),
        std::move(elseBranch)
    );
}

// Parses while loop statements.
std::unique_ptr<Stmt> Parser::parseWhileStatement() {

    // Read and consume the 'while' keyword.
    const Token& whileToken = advance();

    // Check for '('.
    if (peek().type != TokenType::LEFT_PAREN) {
        throw std::runtime_error("Expected '(' after while");
    }

    // Consume '('.
    advance();

    // Parse the loop condition.
    auto condition = parseExpression();

    // Check for ')'.
    if (peek().type != TokenType::RIGHT_PAREN) {
        throw std::runtime_error(
            "Expected ')' after while condition"
        );
    }

    // Consume ')'.
    advance();

    // Check for the opening brace.
    if (peek().type != TokenType::LEFT_BRACE) {
        throw std::runtime_error(
            "Expected '{' after while condition"
        );
    }

    // Parse the loop body.
    auto body = parseBlock();

    // Create and return the WhileStmt AST node.
    return makeLocated<WhileStmt>(
        whileToken,
        std::move(condition),
        std::move(body)
    );
}

// Parses repeat loop statements.
std::unique_ptr<Stmt> Parser::parseRepeatStatement() {

    // Read and consume the 'repeat' keyword.
    const Token& repeatToken = advance();

    // Check for '('.
    if (peek().type != TokenType::LEFT_PAREN) {
        throw std::runtime_error(
            "Expected '(' after repeat"
        );
    }

    // Consume '('.
    advance();

    // Parse the repeat count.
    auto count = parseExpression();

    // Check for ')'.
    if (peek().type != TokenType::RIGHT_PAREN) {
        throw std::runtime_error(
            "Expected ')' after repeat count"
        );
    }

    // Consume ')'.
    advance();

    // Check for the opening brace.
    if (peek().type != TokenType::LEFT_BRACE) {
        throw std::runtime_error(
            "Expected '{' after repeat count"
        );
    }

    // Parse the loop body.
    auto body = parseBlock();

    // Create and return the RepeatStmt AST node.
    return makeLocated<RepeatStmt>(
        repeatToken,
        std::move(count),
        std::move(body)
    );
}

// Parses exit statements.
std::unique_ptr<Stmt> Parser::parseExitStatement() {

    // Read and consume the 'exit' keyword.
    const Token& exitToken = advance();

    // Check for the semicolon.
    if (peek().type != TokenType::SEMICOLON) {
        throw std::runtime_error(
            "Expected ';' after exit"
        );
    }

    // Consume the semicolon.
    advance();

    // Create and return the ExitStmt AST node.
    return makeLocated<ExitStmt>(exitToken);
}

// Parses skip statements.
std::unique_ptr<Stmt> Parser::parseSkipStatement() {

    // Read and consume the 'skip' keyword.
    const Token& skipToken = advance();

    // Check for the semicolon.
    if (peek().type != TokenType::SEMICOLON) {
        throw std::runtime_error(
            "Expected ';' after skip"
        );
    }

    // Consume the semicolon.
    advance();

    // Create and return the SkipStmt AST node.
    return makeLocated<SkipStmt>(skipToken);
}

// Parses function declarations.
std::unique_ptr<Stmt> Parser::parseFunctionDeclaration() {

    // Read and consume the 'func' keyword.
    const Token& funcToken = advance();

    // Check for the function name.
    if (peek().type != TokenType::IDENTIFIER) {
        throw std::runtime_error(
            "Expected function name after 'func'"
        );
    }

    // Read and save the function name.
    std::string name = advance().lexeme;

    // Check for '('.
    if (peek().type != TokenType::LEFT_PAREN) {
        throw std::runtime_error(
            "Expected '(' after function name"
        );
    }

    // Consume '('.
    advance();

    // Store the function parameters.
    std::vector<Parameter> parameters;

    // Check whether the function has parameters.
    if (peek().type != TokenType::RIGHT_PAREN) {

        while (true) {

            // Check and save the parameter type.
            std::string paramType;

            switch (peek().type) {
                case TokenType::TYPE_INT:
                    paramType = "int";
                    break;
                case TokenType::TYPE_FLOAT:
                    paramType = "float";
                    break;
                case TokenType::TYPE_CHAR:
                    paramType = "char";
                    break;
                case TokenType::TYPE_STRING:
                    paramType = "string";
                    break;
                case TokenType::TYPE_BOOL:
                    paramType = "bool";
                    break;
                default:
                    throw std::runtime_error(
                        "Expected parameter type"
                    );
            }

            // Consume the parameter type.
            advance();

            // Check for a parameter name.
            if (peek().type != TokenType::IDENTIFIER) {
                throw std::runtime_error(
                    "Expected parameter name"
                );
            }

            // Read and save the parameter name.
            std::string paramName = advance().lexeme;

            // Add the parameter with its type.
            parameters.emplace_back(
                std::move(paramName),
                std::move(paramType)
            );

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
        throw std::runtime_error(
            "Expected ')' after parameters"
        );
    }

    // Consume ')'.
    advance();

    // Check for the function body.
    if (peek().type != TokenType::LEFT_BRACE) {
        throw std::runtime_error(
            "Expected '{' after function declaration"
        );
    }

    // Parse the function body.
    auto body = parseBlock();

    // Create and return the function AST node.
    return makeLocated<FunctionDeclStmt>(
        funcToken,
        std::move(name),
        std::move(parameters),
        std::move(body)
    );
}

// Parses guard statements.
std::unique_ptr<Stmt> Parser::parseGuardStatement() {

    // Read and consume the 'guard' keyword.
    const Token& guardToken = advance();

    // Parse the guard condition.
    auto condition = parseExpression();

    // Check for the semicolon.
    if (peek().type != TokenType::SEMICOLON) {
        throw std::runtime_error(
            "Expected ';' after guard statement"
        );
    }

    // Consume the semicolon.
    advance();

    // Create and return the GuardStmt AST node.
    return makeLocated<GuardStmt>(
        guardToken,
        std::move(condition)
    );
}

// Parses lock statements.
std::unique_ptr<Stmt> Parser::parseLockStatement() {

    // Read and consume the 'lock' keyword.
    const Token& lockToken = advance();

    // Check for a variable name.
    if (peek().type != TokenType::IDENTIFIER) {
        throw std::runtime_error(
            "Expected variable name after 'lock'"
        );
    }

    // Read and save the variable name.
    std::string name = advance().lexeme;

    // Check for the semicolon.
    if (peek().type != TokenType::SEMICOLON) {
        throw std::runtime_error(
            "Expected ';' after lock statement"
        );
    }

    // Consume the semicolon.
    advance();

    // Create and return the LockStmt AST node.
    return makeLocated<LockStmt>(
        lockToken,
        std::move(name)
    );
}

// Parses unlock statements.
std::unique_ptr<Stmt> Parser::parseUnlockStatement() {

    // Read and consume the 'unlock' keyword.
    const Token& unlockToken = advance();

    // Check for a variable name.
    if (peek().type != TokenType::IDENTIFIER) {
        throw std::runtime_error(
            "Expected variable name after 'unlock'"
        );
    }

    // Read and save the variable name.
    std::string name = advance().lexeme;

    // Check for the semicolon.
    if (peek().type != TokenType::SEMICOLON) {
        throw std::runtime_error(
            "Expected ';' after unlock statement"
        );
    }

    // Consume the semicolon.
    advance();

    // Create and return the UnlockStmt AST node.
    return makeLocated<UnlockStmt>(
        unlockToken,
        std::move(name)
    );
}

// Parses checkpoint statements.
std::unique_ptr<Stmt> Parser::parseCheckpointStatement() {

    // Read and consume the 'checkpoint' keyword.
    const Token& checkpointToken = advance();

    // Check for the checkpoint label.
    if (peek().type != TokenType::IDENTIFIER) {
        throw std::runtime_error(
            "Expected checkpoint label"
        );
    }

    // Read and save the checkpoint label.
    std::string label = advance().lexeme;

    // Check for '('.
    if (peek().type != TokenType::LEFT_PAREN) {
        throw std::runtime_error(
            "Expected '(' after checkpoint label"
        );
    }

    // Consume '('.
    advance();

    // Store the variable names.
    std::vector<std::string> variables;

    // Require at least one variable.
    if (peek().type != TokenType::IDENTIFIER) {
        throw std::runtime_error(
            "Expected variable name in checkpoint"
        );
    }

    // Parse the variable list.
    while (true) {

        // Read and save one variable name.
        variables.push_back(advance().lexeme);

        // Stop if there is no comma.
        if (peek().type != TokenType::COMMA) {
            break;
        }

        // Consume ','.
        advance();

        // Check for the next variable.
        if (peek().type != TokenType::IDENTIFIER) {
            throw std::runtime_error(
                "Expected variable name after ','"
            );
        }
    }

    // Check for ')'.
    if (peek().type != TokenType::RIGHT_PAREN) {
        throw std::runtime_error(
            "Expected ')' after checkpoint variables"
        );
    }

    // Consume ')'.
    advance();

    // Check for the semicolon.
    if (peek().type != TokenType::SEMICOLON) {
        throw std::runtime_error(
            "Expected ';' after checkpoint"
        );
    }

    // Consume the semicolon.
    advance();

    // Create the checkpoint AST node.
    return makeLocated<CheckpointStmt>(
        checkpointToken,
        std::move(label),
        std::move(variables)
    );
}

// Parses restore statements.
std::unique_ptr<Stmt> Parser::parseRestoreStatement() {

    // Read and consume the 'restore' keyword.
    const Token& restoreToken = advance();

    // Check for '('.
    if (peek().type != TokenType::LEFT_PAREN) {
        throw std::runtime_error(
            "Expected '(' after restore"
        );
    }

    // Consume '('.
    advance();

    // Check for the checkpoint label.
    if (peek().type != TokenType::IDENTIFIER) {
        throw std::runtime_error(
            "Expected checkpoint label in restore"
        );
    }

    // Read and save the checkpoint label.
    std::string label = advance().lexeme;

    // Check for ')'.
    if (peek().type != TokenType::RIGHT_PAREN) {
        throw std::runtime_error(
            "Expected ')' after restore label"
        );
    }

    // Consume ')'.
    advance();

    // Check for the semicolon.
    if (peek().type != TokenType::SEMICOLON) {
        throw std::runtime_error(
            "Expected ';' after restore"
        );
    }

    // Consume the semicolon.
    advance();

    // Create the restore AST node.
    return makeLocated<RestoreStmt>(
        restoreToken,
        std::move(label)
    );
}

// Parses budget statements.
std::unique_ptr<Stmt> Parser::parseBudgetStatement() {

    // Read and consume the 'budget' keyword.
    const Token& budgetToken = advance();

    // Check for '('.
    if (peek().type != TokenType::LEFT_PAREN) {
        throw std::runtime_error(
            "Expected '(' after budget"
        );
    }

    // Consume '('.
    advance();

    // Parse the budget limit.
    auto limit = parseExpression();

    // Check for ')'.
    if (peek().type != TokenType::RIGHT_PAREN) {
        throw std::runtime_error(
            "Expected ')' after budget limit"
        );
    }

    // Consume ')'.
    advance();

    // Check for the opening brace.
    if (peek().type != TokenType::LEFT_BRACE) {
        throw std::runtime_error(
            "Expected '{' after budget limit"
        );
    }

    // Parse the budget block.
    auto body = parseBlock();

    // Create and return the BudgetStmt AST node.
    return makeLocated<BudgetStmt>(
        budgetToken,
        std::move(limit),
        std::move(body)
    );
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

        // Save the target's starting position.
        SourceLocation start = left->location;

        // Create the assignment expression.
        auto expression = std::make_unique<AssignmentExpr>(
            std::move(left),
            std::move(right)
        );

        // Record its location.
        expression->location = start;

        return expression;
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

        
        // Save the expression's starting location.
        SourceLocation start = left->location;

        // Create a binary expression.
        left = std::make_unique<BinaryExpr>(
            op,
            std::move(left),
            std::move(right)
        );

        // Record the location on the new node.
        left->location = start;
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

        
        // Save the expression's starting location.
        SourceLocation start = left->location;

        // Create a binary expression.
        left = std::make_unique<BinaryExpr>(
            op,
            std::move(left),
            std::move(right)
        );

        // Record the location on the new node.
        left->location = start;
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

        
        // Save the expression's starting location.
        SourceLocation start = left->location;

        // Create a binary expression.
        left = std::make_unique<BinaryExpr>(
            op,
            std::move(left),
            std::move(right)
        );

        // Record the location on the new node.
        left->location = start;
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

        
        // Save the expression's starting location.
        SourceLocation start = left->location;

        // Create a binary expression.
        left = std::make_unique<BinaryExpr>(
            op,
            std::move(left),
            std::move(right)
        );

        // Record the location on the new node.
        left->location = start;
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

        
        // Save the expression's starting location.
        SourceLocation start = left->location;

        // Create a binary expression.
        left = std::make_unique<BinaryExpr>(
            op,
            std::move(left),
            std::move(right)
        );

        // Record the location on the new node.
        left->location = start;
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

        
        // Save the expression's starting location.
        SourceLocation start = left->location;

        // Create a binary expression.
        left = std::make_unique<BinaryExpr>(
            op,
            std::move(left),
            std::move(right)
        );

        // Record the location on the new node.
        left->location = start;

    }

    // Return the completed expression.
    return left;
}

// Parses unary operators such as - and !.
std::unique_ptr<Expr> Parser::parseUnary() {

    // Check whether the current token is - or !.
    if (peek().type == TokenType::MINUS ||
        peek().type == TokenType::BANG) {

        // Save and consume the unary operator.
        const Token& opToken = advance();
        std::string op = opToken.lexeme;

        // Parse the expression after the operator.
        auto operand = parseUnary();

        // Create and return the unary AST node.
        return makeLocated<UnaryExpr>(
            opToken,
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

        // Save the starting position.
        SourceLocation start = left->location;

        // Create the power expression.
        auto expression = std::make_unique<BinaryExpr>(
            op,
            std::move(left),
            std::move(right)
        );

        // Record its location.
        expression->location = start;

        return expression;
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

            // Save the function name's starting position.
            SourceLocation start = expression->location;

            // Create the call expression.
            expression = std::make_unique<CallExpr>(
                std::move(name),
                std::move(arguments)
            );

            // Record its location.
            expression->location = start;

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

            // Save the collection's starting position.
            SourceLocation start = expression->location;

            // Create the indexing expression.
            expression = std::make_unique<IndexExpr>(
                std::move(expression),
                std::move(index)
            );

            // Record its location.
            expression->location = start;

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
        return makeLocated<IntegerExpr>(token, value);
    }

    // Check whether the current token is a floating-point number.
    if (peek().type == TokenType::FLOAT_LITERAL) {

        // Read and consume the float token.
        const Token& token = advance();

        // Convert the text into a double.
        double value = std::stod(token.lexeme);

        // Create and return the float AST node.
        return makeLocated<FloatExpr>(token, value);
    }

    // Check whether the current token is a string.
    if (peek().type == TokenType::STRING_LITERAL) {

        // Read and consume the string token.
        const Token& token = advance();

        // Get the original string text.
        const std::string& text = token.lexeme;

        // Check the surrounding quotation marks.
        if (text.size() < 2 ||
            text.front() != '"' ||
            text.back() != '"') {

            throw std::runtime_error("Invalid string literal");
        }

        // Store the decoded string value.
        std::string value;

        // Read characters between the quotation marks.
        for (std::size_t i = 1; i + 1 < text.size(); ++i) {

            // Check for an escape sequence.
            if (text[i] == '\\') {

                // Make sure another character follows.
                if (i + 2 >= text.size()) {
                    throw std::runtime_error("Invalid string escape");
                }

                // Decode and store the escaped character.
                value.push_back(decodeEscape(text[++i]));

            } else {

                // Store a normal character.
                value.push_back(text[i]);
            }
        }

        // Create and return the string AST node.
        return makeLocated<StringExpr>(token, std::move(value));
    }

    // Check whether the current token is a Boolean literal.
    if (peek().type == TokenType::BOOL_LITERAL) {

        // Read and consume the Boolean token.
        const Token& token = advance();

        // Convert the text into a Boolean value.
        bool value = (token.lexeme == "true");

        // Create and return the Boolean AST node.
        return makeLocated<BoolExpr>(token, value);
    }

    
    // Check whether the current token is a character.
    if (peek().type == TokenType::CHAR_LITERAL) {

        // Read and consume the character token.
        const Token& token = advance();

        // Get the original character text.
        const std::string& text = token.lexeme;

        // Check the surrounding single quotes.
        if (text.size() < 3 ||
            text.front() != '\'' ||
            text.back() != '\'') {

            throw std::runtime_error("Invalid character literal");
        }

        // Store the character value.
        char value;

        // Normal character, such as 'A'.
        if (text.size() == 3 && text[1] != '\\') {

            value = text[1];

        // Escaped character, such as '\n'.
        } else if (text.size() == 4 && text[1] == '\\') {

            value = decodeEscape(text[2]);

        } else {

            throw std::runtime_error("Invalid character literal");
        }

        // Create and return the character AST node.
        return makeLocated<CharExpr>(token, value);
    }
    
    // Check for an identifier, built-in function or type conversion.
    if (peek().type == TokenType::IDENTIFIER ||
        peek().type == TokenType::INSPECT ||
        peek().type == TokenType::PREVIEW ||
        peek().type == TokenType::INPUT ||
        peek().type == TokenType::OUTPUT ||
        peek().type == TokenType::EXTEND ||
        peek().type == TokenType::TYPE_INT ||
        peek().type == TokenType::TYPE_FLOAT ||
        peek().type == TokenType::TYPE_STRING) {

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
        return makeLocated<VariableExpr>(token, token.lexeme);
    }

    // Check whether the current token is a list constructor.
    if (peek().type == TokenType::TYPE_LIST) {

        // Read and consume 'list'.
        const Token& listToken = advance();

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
        return makeLocated<ListExpr>(
            listToken,
            std::move(elementType),
            std::move(size)
        );
    }

    // Check whether the current token is '('.
    if (peek().type == TokenType::LEFT_PAREN) {

        // Save the opening parenthesis position.
        const Token& openParen = advance();

        // Parse the expression inside the parentheses.
        auto expression = parseExpression();

        // Check for the closing parenthesis.
        if (peek().type != TokenType::RIGHT_PAREN) {
            throw std::runtime_error("Expected ')'");
        }

        // Consume the closing parenthesis.
        advance();

        // Record the opening parenthesis position.
        expression->location = {
            static_cast<int>(openParen.line),
            static_cast<int>(openParen.column)
        };

        // Return the expression inside the parentheses.
        return expression;
    }

    // Report an error for unsupported tokens.
    throw std::runtime_error("Expected an expression");
}


} // namespace gash
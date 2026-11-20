#include "parser.hpp"
#include <utility>

namespace bminor {

Parser::Parser(std::vector<Token> tokens) : tokens_(std::move(tokens)) {}

bool Parser::isAtEnd() const {
    return current_ >= tokens_.size() || tokens_[current_].type == TokenType::END_OF_FILE;
}

const Token& Parser::peek(size_t offset) const {
    size_t idx = current_ + offset;
    if (idx >= tokens_.size()) {
        return tokens_.back();
    }
    return tokens_[idx];
}

const Token& Parser::previous() const {
    if (current_ == 0) return tokens_.front();
    return tokens_[current_ - 1];
}

const Token& Parser::advance() {
    if (!isAtEnd()) {
        current_++;
    }
    return previous();
}

bool Parser::check(TokenType type) const {
    if (isAtEnd()) return type == TokenType::END_OF_FILE;
    return peek().type == type;
}

bool Parser::match(const std::vector<TokenType>& types) {
    for (TokenType t : types) {
        if (check(t)) {
            advance();
            return true;
        }
    }
    return false;
}

bool Parser::expect(TokenType type, const std::string& message) {
    if (check(type)) {
        advance();
        return true;
    }
    reportError(peek().loc, message + " (got '" + peek().lexeme + "')");
    return false;
}

bool Parser::isTypeStart() const {
    switch (peek().type) {
        case TokenType::KW_INTEGER:
        case TokenType::KW_BOOLEAN:
        case TokenType::KW_CHAR:
        case TokenType::KW_STRING:
        case TokenType::KW_FLOAT:
        case TokenType::KW_VOID:
        case TokenType::KW_AUTO:
        case TokenType::KW_ARRAY:
            return true;
        default:
            return false;
    }
}

bool Parser::isSyncPoint() const {
    switch (peek().type) {
        case TokenType::SEMI:
        case TokenType::RBRACE:
        case TokenType::KW_IF:
        case TokenType::KW_WHILE:
        case TokenType::KW_FOR:
        case TokenType::KW_RETURN:
        case TokenType::END_OF_FILE:
            return true;
        default:
            return false;
    }
}

void Parser::reportError(const SourceLocation& loc, const std::string& message) {
    errors_.push_back(ParseError{loc, message});
}

// Panic-Mode Recovery: skip tokens until a synchronization point is reached
void Parser::synchronize() {
    if (check(TokenType::SEMI)) {
        advance();
        return;
    }
    while (!isAtEnd() && !isSyncPoint()) {
        advance();
    }
    if (check(TokenType::SEMI)) {
        advance();
    }
}

std::unique_ptr<ProgramNode> Parser::parseProgram() {
    SourceLocation startLoc = peek().loc;
    auto program = std::make_unique<ProgramNode>(startLoc);

    while (!isAtEnd()) {
        DeclPtr decl = parseDeclaration();
        if (decl) {
            program->declarations.push_back(std::move(decl));
        } else {
            synchronize();
            if (!isAtEnd() && !check(TokenType::IDENTIFIER)) {
                advance();
            }
        }
    }
    return program;
}

DeclPtr Parser::parseDeclaration() {
    if (!check(TokenType::IDENTIFIER)) {
        reportError(peek().loc, "Expected declaration identifier");
        return nullptr;
    }

    Token idTok = advance();
    if (!expect(TokenType::COLON, "Expected ':' after identifier in declaration")) {
        return nullptr;
    }

    if (match({TokenType::KW_FUNCTION})) {
        return parseFuncDecl(idTok.lexeme, idTok.loc);
    }
    return parseVarDecl(idTok.lexeme, idTok.loc);
}

DeclPtr Parser::parseVarDecl(const std::string& name, SourceLocation loc) {
    auto typeSpec = parseTypeSpec();
    if (!typeSpec) {
        return nullptr;
    }

    ExprPtr initExpr = nullptr;
    if (match({TokenType::OP_ASSIGN})) {
        initExpr = parseExpr();
    }

    if (!expect(TokenType::SEMI, "Expected ';' after variable declaration")) {
        synchronize();
    }

    return std::make_unique<VarDeclNode>(std::move(loc), name, std::move(typeSpec), std::move(initExpr));
}

DeclPtr Parser::parseFuncDecl(const std::string& name, SourceLocation loc) {
    auto retType = parseTypeSpec();
    if (!retType) {
        return nullptr;
    }

    if (!expect(TokenType::LPAREN, "Expected '(' in function declaration")) {
        return nullptr;
    }

    std::vector<ParamNode> params;
    if (!check(TokenType::RPAREN)) {
        params = parseParamList();
    }

    if (!expect(TokenType::RPAREN, "Expected ')' after parameter list")) {
        synchronize();
    }

    auto funcNode = std::make_unique<FuncDeclNode>(std::move(loc), name, std::move(retType));
    funcNode->params = std::move(params);

    if (check(TokenType::LBRACE)) {
        funcNode->body = parseBlock();
    } else if (match({TokenType::SEMI})) {
        funcNode->body = nullptr;
    } else {
        reportError(peek().loc, "Expected '{' or ';' after function signature");
        synchronize();
    }

    return funcNode;
}

std::vector<ParamNode> Parser::parseParamList() {
    std::vector<ParamNode> params;
    params.push_back(parseParam());
    while (match({TokenType::COMMA})) {
        params.push_back(parseParam());
    }
    return params;
}

ParamNode Parser::parseParam() {
    SourceLocation loc = peek().loc;
    std::string paramName;
    if (check(TokenType::IDENTIFIER)) {
        paramName = advance().lexeme;
    } else {
        reportError(loc, "Expected parameter name");
    }
    expect(TokenType::COLON, "Expected ':' after parameter name");
    auto typeSpec = parseTypeSpec();
    return ParamNode(std::move(loc), paramName, std::move(typeSpec));
}

std::unique_ptr<TypeNode> Parser::parseTypeSpec() {
    SourceLocation loc = peek().loc;
    if (match({TokenType::KW_ARRAY})) {
        std::unique_ptr<ASTNode> sizeExpr = nullptr;
        if (expect(TokenType::LBRACKET, "Expected '[' after 'array'")) {
            if (!check(TokenType::RBRACKET)) {
                sizeExpr = parseExpr();
            }
            expect(TokenType::RBRACKET, "Expected ']' after array size");
        }
        auto elemType = parseTypeSpec();
        std::string base = elemType ? elemType->baseName : "integer";
        auto arrNode = std::make_unique<TypeNode>(std::move(loc), base, true);
        arrNode->arraySize = std::move(sizeExpr);
        return arrNode;
    }

    if (isTypeStart()) {
        Token t = advance();
        return std::make_unique<TypeNode>(std::move(loc), t.lexeme, false);
    }

    reportError(loc, "Expected type specifier");
    return nullptr;
}

std::unique_ptr<BlockNode> Parser::parseBlock() {
    SourceLocation loc = peek().loc;
    expect(TokenType::LBRACE, "Expected '{' to start block");
    auto block = std::make_unique<BlockNode>(std::move(loc));

    while (!isAtEnd() && !check(TokenType::RBRACE)) {
        size_t beforePos = current_;
        StmtPtr stmt = parseStatement();
        if (stmt) {
            block->statements.push_back(std::move(stmt));
        } else {
            synchronize();
            // Guarantee progress even if parser is stuck on an unexpected token
            if (current_ == beforePos && !isAtEnd() && !check(TokenType::RBRACE)) {
                advance();
            }
        }
    }

    expect(TokenType::RBRACE, "Expected '}' to close block");
    return block;
}

StmtPtr Parser::parseStatement() {
    if (check(TokenType::KW_IF)) return parseIfStmt();
    if (check(TokenType::KW_WHILE)) return parseWhileStmt();
    if (check(TokenType::KW_FOR)) return parseForStmt();
    if (check(TokenType::KW_RETURN)) return parseReturnStmt();
    if (check(TokenType::LBRACE)) return parseBlock();

    // Local variable declaration starts with: IDENT ':' type
    if (check(TokenType::IDENTIFIER) && peek(1).type == TokenType::COLON) {
        SourceLocation loc = peek().loc;
        std::string name = advance().lexeme;
        advance(); // consume ':'
        DeclPtr decl = parseVarDecl(name, std::move(loc));
        if (!decl) return nullptr;
        auto wrapper = std::make_unique<ExprStmtNode>(decl->loc, nullptr);
        return wrapper;
    }

    return parseExprStmt();
}

StmtPtr Parser::parseIfStmt() {
    SourceLocation loc = advance().loc; // consume 'if'
    expect(TokenType::LPAREN, "Expected '(' after 'if'");
    ExprPtr cond = parseExpr();
    expect(TokenType::RPAREN, "Expected ')' after if condition");
    StmtPtr thenBranch = parseStatement();

    StmtPtr elseBranch = nullptr;
    if (match({TokenType::KW_ELSE})) {
        elseBranch = parseStatement();
    }

    return std::make_unique<IfStmtNode>(std::move(loc), std::move(cond), std::move(thenBranch), std::move(elseBranch));
}

StmtPtr Parser::parseWhileStmt() {
    SourceLocation loc = advance().loc; // consume 'while'
    expect(TokenType::LPAREN, "Expected '(' after 'while'");
    ExprPtr cond = parseExpr();
    expect(TokenType::RPAREN, "Expected ')' after while condition");
    StmtPtr body = parseStatement();
    return std::make_unique<WhileStmtNode>(std::move(loc), std::move(cond), std::move(body));
}

StmtPtr Parser::parseForStmt() {
    SourceLocation loc = advance().loc; // consume 'for'
    expect(TokenType::LPAREN, "Expected '(' after 'for'");

    ExprPtr initExpr = nullptr;
    if (!check(TokenType::SEMI)) {
        initExpr = parseExpr();
    }
    expect(TokenType::SEMI, "Expected ';' after for-loop initialization");

    ExprPtr condExpr = nullptr;
    if (!check(TokenType::SEMI)) {
        condExpr = parseExpr();
    }
    expect(TokenType::SEMI, "Expected ';' after for-loop condition");

    ExprPtr updateExpr = nullptr;
    if (!check(TokenType::RPAREN)) {
        updateExpr = parseExpr();
    }
    expect(TokenType::RPAREN, "Expected ')' after for-loop clauses");

    StmtPtr body = parseStatement();
    return std::make_unique<ForStmtNode>(
        std::move(loc), std::move(initExpr), std::move(condExpr), std::move(updateExpr), std::move(body));
}

StmtPtr Parser::parseReturnStmt() {
    SourceLocation loc = advance().loc; // consume 'return'
    ExprPtr val = nullptr;
    if (!check(TokenType::SEMI)) {
        val = parseExpr();
    }
    if (!expect(TokenType::SEMI, "Expected ';' after return statement")) {
        synchronize();
    }
    return std::make_unique<ReturnStmtNode>(std::move(loc), std::move(val));
}

StmtPtr Parser::parseExprStmt() {
    SourceLocation loc = peek().loc;
    ExprPtr expr = nullptr;
    if (!check(TokenType::SEMI)) {
        expr = parseExpr();
    }
    if (!expect(TokenType::SEMI, "Expected ';' at end of expression statement")) {
        synchronize();
    }
    return std::make_unique<ExprStmtNode>(std::move(loc), std::move(expr));
}

ExprPtr Parser::parseExpr() {
    return parseAssignment();
}

ExprPtr Parser::parseAssignment() {
    ExprPtr lhs = parseLogicalOr();
    if (match({TokenType::OP_ASSIGN})) {
        SourceLocation loc = previous().loc;
        ExprPtr rhs = parseAssignment();
        return std::make_unique<AssignExprNode>(std::move(loc), std::move(lhs), std::move(rhs));
    }
    return lhs;
}

ExprPtr Parser::parseLogicalOr() {
    ExprPtr expr = parseLogicalAnd();
    while (match({TokenType::OP_OR})) {
        SourceLocation loc = previous().loc;
        std::string op = previous().lexeme;
        ExprPtr right = parseLogicalAnd();
        expr = std::make_unique<BinaryExprNode>(std::move(loc), op, std::move(expr), std::move(right));
    }
    return expr;
}

ExprPtr Parser::parseLogicalAnd() {
    ExprPtr expr = parseEquality();
    while (match({TokenType::OP_AND})) {
        SourceLocation loc = previous().loc;
        std::string op = previous().lexeme;
        ExprPtr right = parseEquality();
        expr = std::make_unique<BinaryExprNode>(std::move(loc), op, std::move(expr), std::move(right));
    }
    return expr;
}

ExprPtr Parser::parseEquality() {
    ExprPtr expr = parseRelational();
    while (match({TokenType::OP_EQ, TokenType::OP_NE})) {
        SourceLocation loc = previous().loc;
        std::string op = previous().lexeme;
        ExprPtr right = parseRelational();
        expr = std::make_unique<BinaryExprNode>(std::move(loc), op, std::move(expr), std::move(right));
    }
    return expr;
}

ExprPtr Parser::parseRelational() {
    ExprPtr expr = parseAdditive();
    while (match({TokenType::OP_LT, TokenType::OP_LE, TokenType::OP_GT, TokenType::OP_GE})) {
        SourceLocation loc = previous().loc;
        std::string op = previous().lexeme;
        ExprPtr right = parseAdditive();
        expr = std::make_unique<BinaryExprNode>(std::move(loc), op, std::move(expr), std::move(right));
    }
    return expr;
}

ExprPtr Parser::parseAdditive() {
    ExprPtr expr = parseMultiplicative();
    while (match({TokenType::OP_PLUS, TokenType::OP_MINUS})) {
        SourceLocation loc = previous().loc;
        std::string op = previous().lexeme;
        ExprPtr right = parseMultiplicative();
        expr = std::make_unique<BinaryExprNode>(std::move(loc), op, std::move(expr), std::move(right));
    }
    return expr;
}

ExprPtr Parser::parseMultiplicative() {
    ExprPtr expr = parseUnary();
    while (match({TokenType::OP_STAR, TokenType::OP_SLASH, TokenType::OP_MOD})) {
        SourceLocation loc = previous().loc;
        std::string op = previous().lexeme;
        ExprPtr right = parseUnary();
        expr = std::make_unique<BinaryExprNode>(std::move(loc), op, std::move(expr), std::move(right));
    }
    return expr;
}

ExprPtr Parser::parseUnary() {
    if (match({TokenType::OP_MINUS, TokenType::OP_NOT})) {
        SourceLocation loc = previous().loc;
        std::string op = previous().lexeme;
        ExprPtr operand = parseUnary();
        return std::make_unique<UnaryExprNode>(std::move(loc), op, std::move(operand));
    }
    return parsePostfix();
}

ExprPtr Parser::parsePostfix() {
    ExprPtr expr = parsePrimary();
    while (true) {
        if (match({TokenType::LBRACKET})) {
            SourceLocation loc = previous().loc;
            ExprPtr idx = parseExpr();
            expect(TokenType::RBRACKET, "Expected ']' after array index");
            expr = std::make_unique<ArrayIndexExprNode>(std::move(loc), std::move(expr), std::move(idx));
        } else if (match({TokenType::LPAREN})) {
            SourceLocation loc = previous().loc;
            std::vector<ExprPtr> args;
            if (!check(TokenType::RPAREN)) {
                args = parseArgList();
            }
            expect(TokenType::RPAREN, "Expected ')' after call arguments");

            std::string calleeName = "<expr>";
            if (auto* idNode = dynamic_cast<IdentifierNode*>(expr.get())) {
                calleeName = idNode->name;
            }
            expr = std::make_unique<CallExprNode>(std::move(loc), calleeName, std::move(args));
        } else if (match({TokenType::OP_PLUS_PLUS, TokenType::OP_MINUS_MINUS})) {
            SourceLocation loc = previous().loc;
            std::string op = previous().lexeme;
            expr = std::make_unique<PostfixExprNode>(std::move(loc), op, std::move(expr));
        } else {
            break;
        }
    }
    return expr;
}

std::vector<ExprPtr> Parser::parseArgList() {
    std::vector<ExprPtr> args;
    args.push_back(parseExpr());
    while (match({TokenType::COMMA})) {
        args.push_back(parseExpr());
    }
    return args;
}

ExprPtr Parser::parsePrimary() {
    SourceLocation loc = peek().loc;

    if (match({TokenType::INT_LITERAL})) {
        return std::make_unique<IntLiteralNode>(std::move(loc), previous().lexeme);
    }
    if (match({TokenType::FLOAT_LITERAL})) {
        return std::make_unique<FloatLiteralNode>(std::move(loc), previous().lexeme);
    }
    if (match({TokenType::CHAR_LITERAL})) {
        return std::make_unique<CharLiteralNode>(std::move(loc), previous().lexeme);
    }
    if (match({TokenType::STRING_LITERAL})) {
        return std::make_unique<StringLiteralNode>(std::move(loc), previous().lexeme);
    }
    if (match({TokenType::KW_TRUE})) {
        return std::make_unique<BoolLiteralNode>(std::move(loc), true);
    }
    if (match({TokenType::KW_FALSE})) {
        return std::make_unique<BoolLiteralNode>(std::move(loc), false);
    }
    if (match({TokenType::IDENTIFIER})) {
        return std::make_unique<IdentifierNode>(std::move(loc), previous().lexeme);
    }
    if (match({TokenType::LPAREN})) {
        ExprPtr inner = parseExpr();
        expect(TokenType::RPAREN, "Expected ')' after parenthesized expression");
        return inner;
    }

    reportError(loc, "Expected expression (got '" + peek().lexeme + "')");
    return nullptr;
}

} // namespace bminor
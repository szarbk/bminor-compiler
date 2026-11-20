#ifndef BMINOR_PARSER_HPP
#define BMINOR_PARSER_HPP

#include "ast.hpp"
#include "token.hpp"
#include <string>
#include <vector>

namespace bminor {

struct ParseError {
    SourceLocation loc;
    std::string message;
};

class Parser {
public:
    explicit Parser(std::vector<Token> tokens);

    std::unique_ptr<ProgramNode> parseProgram();
    const std::vector<ParseError>& errors() const { return errors_; }

private:
    std::vector<Token> tokens_;
    size_t current_ = 0;
    std::vector<ParseError> errors_;

    bool isAtEnd() const;
    const Token& peek(size_t offset = 0) const;
    const Token& previous() const;
    const Token& advance();
    bool check(TokenType type) const;
    bool match(const std::vector<TokenType>& types);
    bool expect(TokenType type, const std::string& message);

    bool isTypeStart() const;
    bool isSyncPoint() const;
    void reportError(const SourceLocation& loc, const std::string& message);
    void synchronize();

    // Grammar rules (LL / Recursive Descent)
    DeclPtr parseDeclaration();
    DeclPtr parseVarDecl(const std::string& name, SourceLocation loc);
    DeclPtr parseFuncDecl(const std::string& name, SourceLocation loc);
    std::vector<ParamNode> parseParamList();
    ParamNode parseParam();

    std::unique_ptr<TypeNode> parseTypeSpec();
    std::unique_ptr<BlockNode> parseBlock();
    StmtPtr parseStatement();
    StmtPtr parseIfStmt();
    StmtPtr parseWhileStmt();
    StmtPtr parseForStmt();
    StmtPtr parseReturnStmt();
    StmtPtr parseExprStmt();

    ExprPtr parseExpr();
    ExprPtr parseAssignment();
    ExprPtr parseLogicalOr();
    ExprPtr parseLogicalAnd();
    ExprPtr parseEquality();
    ExprPtr parseRelational();
    ExprPtr parseAdditive();
    ExprPtr parseMultiplicative();
    ExprPtr parseUnary();
    ExprPtr parsePostfix();
    std::vector<ExprPtr> parseArgList();
    ExprPtr parsePrimary();
};

} // namespace bminor

#endif // BMINOR_PARSER_HPP
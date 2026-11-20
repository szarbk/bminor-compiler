#ifndef BMINOR_AST_HPP
#define BMINOR_AST_HPP

#include "token.hpp"
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace bminor {

enum class ASTNodeKind {
    Program,
    VarDecl,
    FuncDecl,
    Param,
    TypeSpec,
    Block,
    IfStmt,
    WhileStmt,
    ForStmt,
    ReturnStmt,
    ExprStmt,
    AssignExpr,
    BinaryExpr,
    UnaryExpr,
    PostfixExpr,
    CallExpr,
    ArrayIndexExpr,
    IntLiteral,
    FloatLiteral,
    CharLiteral,
    StringLiteral,
    BoolLiteral,
    Identifier
};

struct ASTNode {
    ASTNodeKind kind;
    SourceLocation loc;
    // Populated in later semantic analysis phases; left empty (std::nullopt) in Phase 1
    std::optional<std::string> typeAnnotation;

    ASTNode(ASTNodeKind k, SourceLocation location)
        : kind(k), loc(std::move(location)), typeAnnotation(std::nullopt) {}
    virtual ~ASTNode() = default;
};

using ASTNodePtr = std::unique_ptr<ASTNode>;

struct TypeNode : ASTNode {
    std::string baseName;
    bool isArray = false;
    std::unique_ptr<ASTNode> arraySize; // optional expression

    TypeNode(SourceLocation loc, std::string base, bool arr = false)
        : ASTNode(ASTNodeKind::TypeSpec, std::move(loc)),
          baseName(std::move(base)),
          isArray(arr) {}
};

struct ExprNode : ASTNode {
    using ASTNode::ASTNode;
};

using ExprPtr = std::unique_ptr<ExprNode>;

struct StmtNode : ASTNode {
    using ASTNode::ASTNode;
};

using StmtPtr = std::unique_ptr<StmtNode>;

struct DeclNode : ASTNode {
    std::string name;
    std::unique_ptr<TypeNode> type;

    DeclNode(ASTNodeKind k, SourceLocation loc, std::string n, std::unique_ptr<TypeNode> t)
        : ASTNode(k, std::move(loc)), name(std::move(n)), type(std::move(t)) {}
};

using DeclPtr = std::unique_ptr<DeclNode>;

struct ParamNode : ASTNode {
    std::string name;
    std::unique_ptr<TypeNode> type;

    ParamNode(SourceLocation loc, std::string n, std::unique_ptr<TypeNode> t)
        : ASTNode(ASTNodeKind::Param, std::move(loc)), name(std::move(n)), type(std::move(t)) {}
};

struct VarDeclNode : DeclNode {
    ExprPtr initializer; // optional

    VarDeclNode(SourceLocation loc, std::string n, std::unique_ptr<TypeNode> t, ExprPtr init = nullptr)
        : DeclNode(ASTNodeKind::VarDecl, std::move(loc), std::move(n), std::move(t)),
          initializer(std::move(init)) {}
};

struct BlockNode : StmtNode {
    std::vector<StmtPtr> statements;

    explicit BlockNode(SourceLocation loc)
        : StmtNode(ASTNodeKind::Block, std::move(loc)) {}
};

struct FuncDeclNode : DeclNode {
    std::vector<ParamNode> params;
    std::unique_ptr<BlockNode> body; // nullptr for prototype/declaration ending with ';'

    FuncDeclNode(SourceLocation loc, std::string n, std::unique_ptr<TypeNode> retType)
        : DeclNode(ASTNodeKind::FuncDecl, std::move(loc), std::move(n), std::move(retType)) {}
};

struct ProgramNode : ASTNode {
    std::vector<DeclPtr> declarations;

    explicit ProgramNode(SourceLocation loc)
        : ASTNode(ASTNodeKind::Program, std::move(loc)) {}
};

// Statements
struct IfStmtNode : StmtNode {
    ExprPtr condition;
    StmtPtr thenBranch;
    StmtPtr elseBranch; // optional

    IfStmtNode(SourceLocation loc, ExprPtr cond, StmtPtr thenB, StmtPtr elseB = nullptr)
        : StmtNode(ASTNodeKind::IfStmt, std::move(loc)),
          condition(std::move(cond)),
          thenBranch(std::move(thenB)),
          elseBranch(std::move(elseB)) {}
};

struct WhileStmtNode : StmtNode {
    ExprPtr condition;
    StmtPtr body;

    WhileStmtNode(SourceLocation loc, ExprPtr cond, StmtPtr b)
        : StmtNode(ASTNodeKind::WhileStmt, std::move(loc)),
          condition(std::move(cond)),
          body(std::move(b)) {}
};

struct ForStmtNode : StmtNode {
    ExprPtr initExpr;
    ExprPtr condExpr;
    ExprPtr updateExpr;
    StmtPtr body;

    ForStmtNode(SourceLocation loc, ExprPtr initE, ExprPtr condE, ExprPtr updE, StmtPtr b)
        : StmtNode(ASTNodeKind::ForStmt, std::move(loc)),
          initExpr(std::move(initE)),
          condExpr(std::move(condE)),
          updateExpr(std::move(updE)),
          body(std::move(b)) {}
};

struct ReturnStmtNode : StmtNode {
    ExprPtr value; // optional

    ReturnStmtNode(SourceLocation loc, ExprPtr val = nullptr)
        : StmtNode(ASTNodeKind::ReturnStmt, std::move(loc)), value(std::move(val)) {}
};

struct ExprStmtNode : StmtNode {
    ExprPtr expr; // optional

    ExprStmtNode(SourceLocation loc, ExprPtr e = nullptr)
        : StmtNode(ASTNodeKind::ExprStmt, std::move(loc)), expr(std::move(e)) {}
};

// Expressions
struct IntLiteralNode : ExprNode {
    std::string rawValue;
    IntLiteralNode(SourceLocation loc, std::string raw)
        : ExprNode(ASTNodeKind::IntLiteral, std::move(loc)), rawValue(std::move(raw)) {}
};

struct FloatLiteralNode : ExprNode {
    std::string rawValue;
    FloatLiteralNode(SourceLocation loc, std::string raw)
        : ExprNode(ASTNodeKind::FloatLiteral, std::move(loc)), rawValue(std::move(raw)) {}
};

struct CharLiteralNode : ExprNode {
    std::string rawValue;
    CharLiteralNode(SourceLocation loc, std::string raw)
        : ExprNode(ASTNodeKind::CharLiteral, std::move(loc)), rawValue(std::move(raw)) {}
};

struct StringLiteralNode : ExprNode {
    std::string rawValue;
    StringLiteralNode(SourceLocation loc, std::string raw)
        : ExprNode(ASTNodeKind::StringLiteral, std::move(loc)), rawValue(std::move(raw)) {}
};

struct BoolLiteralNode : ExprNode {
    bool value;
    BoolLiteralNode(SourceLocation loc, bool v)
        : ExprNode(ASTNodeKind::BoolLiteral, std::move(loc)), value(v) {}
};

struct IdentifierNode : ExprNode {
    std::string name;
    IdentifierNode(SourceLocation loc, std::string n)
        : ExprNode(ASTNodeKind::Identifier, std::move(loc)), name(std::move(n)) {}
};

struct UnaryExprNode : ExprNode {
    std::string op;
    ExprPtr operand;

    UnaryExprNode(SourceLocation loc, std::string oper, ExprPtr rhs)
        : ExprNode(ASTNodeKind::UnaryExpr, std::move(loc)), op(std::move(oper)), operand(std::move(rhs)) {}
};

struct BinaryExprNode : ExprNode {
    std::string op;
    ExprPtr left;
    ExprPtr right;

    BinaryExprNode(SourceLocation loc, std::string oper, ExprPtr lhs, ExprPtr rhs)
        : ExprNode(ASTNodeKind::BinaryExpr, std::move(loc)),
          op(std::move(oper)),
          left(std::move(lhs)),
          right(std::move(rhs)) {}
};

struct AssignExprNode : ExprNode {
    ExprPtr target;
    ExprPtr value;

    AssignExprNode(SourceLocation loc, ExprPtr lhs, ExprPtr rhs)
        : ExprNode(ASTNodeKind::AssignExpr, std::move(loc)),
          target(std::move(lhs)),
          value(std::move(rhs)) {}
};

struct CallExprNode : ExprNode {
    std::string callee;
    std::vector<ExprPtr> args;

    CallExprNode(SourceLocation loc, std::string fnName, std::vector<ExprPtr> arguments)
        : ExprNode(ASTNodeKind::CallExpr, std::move(loc)),
          callee(std::move(fnName)),
          args(std::move(arguments)) {}
};

struct ArrayIndexExprNode : ExprNode {
    ExprPtr arrayExpr;
    ExprPtr indexExpr;

    ArrayIndexExprNode(SourceLocation loc, ExprPtr arr, ExprPtr idx)
        : ExprNode(ASTNodeKind::ArrayIndexExpr, std::move(loc)),
          arrayExpr(std::move(arr)),
          indexExpr(std::move(idx)) {}
};

struct PostfixExprNode : ExprNode {
    std::string op; // "++" or "--"
    ExprPtr operand;

    PostfixExprNode(SourceLocation loc, std::string oper, ExprPtr expr)
        : ExprNode(ASTNodeKind::PostfixExpr, std::move(loc)), op(std::move(oper)), operand(std::move(expr)) {}
};

} // namespace bminor

#endif // BMINOR_AST_HPP
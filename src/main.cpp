#include "ast.hpp"
#include "highlighter.hpp"
#include "lexer.hpp"
#include "parser.hpp"
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

namespace {

std::string readFile(const std::string& path, bool& ok) {
    std::ifstream in(path, std::ios::in | std::ios::binary);
    if (!in) {
        ok = false;
        return "";
    }
    std::ostringstream ss;
    ss << in.rdbuf();
    ok = true;
    return ss.str();
}

bool writeFile(const std::string& path, const std::string& content) {
    std::ofstream out(path, std::ios::out | std::ios::binary);
    if (!out) return false;
    out << content;
    return true;
}

void printIndent(int depth) {
    for (int i = 0; i < depth; ++i) {
        std::cout << "  ";
    }
}

void printExprAST(const bminor::ExprNode* expr, int depth) {
    if (!expr) {
        printIndent(depth);
        std::cout << "<null-expr>\n";
        return;
    }
    printIndent(depth);
    switch (expr->kind) {
        case bminor::ASTNodeKind::IntLiteral: {
            const auto* n = static_cast<const bminor::IntLiteralNode*>(expr);
            std::cout << "IntLiteral(" << n->rawValue << ") @" << n->loc.line << ":" << n->loc.column << "\n";
            break;
        }
        case bminor::ASTNodeKind::FloatLiteral: {
            const auto* n = static_cast<const bminor::FloatLiteralNode*>(expr);
            std::cout << "FloatLiteral(" << n->rawValue << ") @" << n->loc.line << ":" << n->loc.column << "\n";
            break;
        }
        case bminor::ASTNodeKind::CharLiteral: {
            const auto* n = static_cast<const bminor::CharLiteralNode*>(expr);
            std::cout << "CharLiteral(" << n->rawValue << ") @" << n->loc.line << ":" << n->loc.column << "\n";
            break;
        }
        case bminor::ASTNodeKind::StringLiteral: {
            const auto* n = static_cast<const bminor::StringLiteralNode*>(expr);
            std::cout << "StringLiteral(" << n->rawValue << ") @" << n->loc.line << ":" << n->loc.column << "\n";
            break;
        }
        case bminor::ASTNodeKind::BoolLiteral: {
            const auto* n = static_cast<const bminor::BoolLiteralNode*>(expr);
            std::cout << "BoolLiteral(" << (n->value ? "true" : "false") << ") @" << n->loc.line << ":" << n->loc.column << "\n";
            break;
        }
        case bminor::ASTNodeKind::Identifier: {
            const auto* n = static_cast<const bminor::IdentifierNode*>(expr);
            std::cout << "Identifier(" << n->name << ") @" << n->loc.line << ":" << n->loc.column << "\n";
            break;
        }
        case bminor::ASTNodeKind::UnaryExpr: {
            const auto* n = static_cast<const bminor::UnaryExprNode*>(expr);
            std::cout << "UnaryExpr(op='" << n->op << "') @" << n->loc.line << ":" << n->loc.column << "\n";
            printExprAST(n->operand.get(), depth + 1);
            break;
        }
        case bminor::ASTNodeKind::BinaryExpr: {
            const auto* n = static_cast<const bminor::BinaryExprNode*>(expr);
            std::cout << "BinaryExpr(op='" << n->op << "') @" << n->loc.line << ":" << n->loc.column << "\n";
            printExprAST(n->left.get(), depth + 1);
            printExprAST(n->right.get(), depth + 1);
            break;
        }
        case bminor::ASTNodeKind::AssignExpr: {
            const auto* n = static_cast<const bminor::AssignExprNode*>(expr);
            std::cout << "AssignExpr @" << n->loc.line << ":" << n->loc.column << "\n";
            printExprAST(n->target.get(), depth + 1);
            printExprAST(n->value.get(), depth + 1);
            break;
        }
        case bminor::ASTNodeKind::CallExpr: {
            const auto* n = static_cast<const bminor::CallExprNode*>(expr);
            std::cout << "CallExpr(callee='" << n->callee << "') @" << n->loc.line << ":" << n->loc.column << "\n";
            for (const auto& arg : n->args) {
                printExprAST(arg.get(), depth + 1);
            }
            break;
        }
        case bminor::ASTNodeKind::ArrayIndexExpr: {
            const auto* n = static_cast<const bminor::ArrayIndexExprNode*>(expr);
            std::cout << "ArrayIndexExpr @" << n->loc.line << ":" << n->loc.column << "\n";
            printExprAST(n->arrayExpr.get(), depth + 1);
            printExprAST(n->indexExpr.get(), depth + 1);
            break;
        }
        case bminor::ASTNodeKind::PostfixExpr: {
            const auto* n = static_cast<const bminor::PostfixExprNode*>(expr);
            std::cout << "PostfixExpr(op='" << n->op << "') @" << n->loc.line << ":" << n->loc.column << "\n";
            printExprAST(n->operand.get(), depth + 1);
            break;
        }
        default:
            std::cout << "ExprNode @" << expr->loc.line << ":" << expr->loc.column << "\n";
            break;
    }
}

void printStmtAST(const bminor::StmtNode* stmt, int depth) {
    if (!stmt) return;
    printIndent(depth);
    switch (stmt->kind) {
        case bminor::ASTNodeKind::Block: {
            const auto* b = static_cast<const bminor::BlockNode*>(stmt);
            std::cout << "Block @" << b->loc.line << ":" << b->loc.column << "\n";
            for (const auto& s : b->statements) {
                printStmtAST(s.get(), depth + 1);
            }
            break;
        }
        case bminor::ASTNodeKind::IfStmt: {
            const auto* s = static_cast<const bminor::IfStmtNode*>(stmt);
            std::cout << "IfStmt @" << s->loc.line << ":" << s->loc.column << "\n";
            printExprAST(s->condition.get(), depth + 1);
            printStmtAST(s->thenBranch.get(), depth + 1);
            if (s->elseBranch) {
                printStmtAST(s->elseBranch.get(), depth + 1);
            }
            break;
        }
        case bminor::ASTNodeKind::WhileStmt: {
            const auto* s = static_cast<const bminor::WhileStmtNode*>(stmt);
            std::cout << "WhileStmt @" << s->loc.line << ":" << s->loc.column << "\n";
            printExprAST(s->condition.get(), depth + 1);
            printStmtAST(s->body.get(), depth + 1);
            break;
        }
        case bminor::ASTNodeKind::ForStmt: {
            const auto* s = static_cast<const bminor::ForStmtNode*>(stmt);
            std::cout << "ForStmt @" << s->loc.line << ":" << s->loc.column << "\n";
            if (s->initExpr) printExprAST(s->initExpr.get(), depth + 1);
            if (s->condExpr) printExprAST(s->condExpr.get(), depth + 1);
            if (s->updateExpr) printExprAST(s->updateExpr.get(), depth + 1);
            printStmtAST(s->body.get(), depth + 1);
            break;
        }
        case bminor::ASTNodeKind::ReturnStmt: {
            const auto* s = static_cast<const bminor::ReturnStmtNode*>(stmt);
            std::cout << "ReturnStmt @" << s->loc.line << ":" << s->loc.column << "\n";
            if (s->value) printExprAST(s->value.get(), depth + 1);
            break;
        }
        case bminor::ASTNodeKind::ExprStmt: {
            const auto* s = static_cast<const bminor::ExprStmtNode*>(stmt);
            std::cout << "ExprStmt @" << s->loc.line << ":" << s->loc.column << "\n";
            if (s->expr) printExprAST(s->expr.get(), depth + 1);
            break;
        }
        default:
            std::cout << "StmtNode @" << stmt->loc.line << ":" << stmt->loc.column << "\n";
            break;
    }
}

void printProgramAST(const bminor::ProgramNode* prog) {
    if (!prog) return;
    std::cout << "=== AST ===\nProgram @" << prog->loc.line << ":" << prog->loc.column << "\n";
    for (const auto& decl : prog->declarations) {
        if (!decl) continue;
        if (decl->kind == bminor::ASTNodeKind::VarDecl) {
            const auto* vd = static_cast<const bminor::VarDeclNode*>(decl.get());
            std::cout << "  VarDecl(name='" << vd->name
                      << "', type='" << (vd->type ? vd->type->baseName : "")
                      << (vd->type && vd->type->isArray ? "[]" : "")
                      << "') @" << vd->loc.line << ":" << vd->loc.column << "\n";
            if (vd->initializer) {
                printExprAST(vd->initializer.get(), 2);
            }
        } else if (decl->kind == bminor::ASTNodeKind::FuncDecl) {
            const auto* fd = static_cast<const bminor::FuncDeclNode*>(decl.get());
            std::cout << "  FuncDecl(name='" << fd->name
                      << "', return='" << (fd->type ? fd->type->baseName : "")
                      << "') @" << fd->loc.line << ":" << fd->loc.column << "\n";
            if (fd->body) {
                printStmtAST(fd->body.get(), 2);
            }
        }
    }
}

} // namespace

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: bminor_front <source_file> [output_html_file]\n";
        return 1;
    }

    std::string inputPath = argv[1];
    std::string htmlOutputPath = (argc >= 3) ? argv[2] : "highlighted_output.html";

    bool ok = false;
    std::string source = readFile(inputPath, ok);
    if (!ok) {
        std::cerr << "Error: Could not open input file '" << inputPath << "'\n";
        return 1;
    }

    bminor::Lexer lexer(source, inputPath);
    std::vector<bminor::Token> tokens = lexer.tokenize();

    std::cout << "=== Tokens (" << tokens.size() << ") ===\n";
    for (const auto& tok : tokens) {
        std::cout << "  " << bminor::tokenTypeToString(tok.type)
                  << "('" << tok.lexeme << "') @ "
                  << tok.loc.line << ":" << tok.loc.column << "\n";
    }

    if (!lexer.errors().empty()) {
        std::cerr << "\n=== Lexer Errors (" << lexer.errors().size() << ") ===\n";
        for (const auto& err : lexer.errors()) {
            std::cerr << "  [" << err.loc.filename << ":" << err.loc.line << ":" << err.loc.column
                      << "] " << err.message << "\n";
        }
    }

    bminor::Parser parser(tokens);
    std::unique_ptr<bminor::ProgramNode> ast = parser.parseProgram();

    printProgramAST(ast.get());

    if (!parser.errors().empty()) {
        std::cerr << "\n=== Parser Errors (" << parser.errors().size() << ") ===\n";
        for (const auto& err : parser.errors()) {
            std::cerr << "  [" << err.loc.line << ":" << err.loc.column
                      << "] " << err.message << "\n";
        }
    }

    bminor::SyntaxHighlighter highlighter(source, tokens, ast.get());
    std::cout << "\n=== ANSI Highlighted Source ===\n";
    std::cout << highlighter.renderAnsi() << "\n";

    std::string html = highlighter.renderHtml("B-Minor: " + inputPath);
    if (writeFile(htmlOutputPath, html)) {
        std::cout << "\nHTML highlighted output written to: " << htmlOutputPath << "\n";
    } else {
        std::cerr << "\nWarning: Failed to write HTML output to '" << htmlOutputPath << "'\n";
    }

    return 0;
}
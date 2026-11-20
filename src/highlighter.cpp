#include "highlighter.hpp"
#include <sstream>
#include <utility>

namespace bminor {

SyntaxHighlighter::SyntaxHighlighter(std::string source,
                                     std::vector<Token> tokens,
                                     const ProgramNode* astRoot)
    : source_(std::move(source)), tokens_(std::move(tokens)) {
    if (astRoot) {
        collectFunctionLocations(astRoot);
    }
}

std::string SyntaxHighlighter::makeLocKey(int line, int column) {
    return std::to_string(line) + ":" + std::to_string(column);
}

void SyntaxHighlighter::collectFunctionLocations(const ProgramNode* root) {
    for (const auto& decl : root->declarations) {
        collectFromDecl(decl.get());
    }
}

void SyntaxHighlighter::collectFromDecl(const DeclNode* decl) {
    if (!decl) return;
    if (decl->kind == ASTNodeKind::FuncDecl) {
        const auto* fn = static_cast<const FuncDeclNode*>(decl);
        functionLocationKeys_.insert(makeLocKey(fn->loc.line, fn->loc.column));
        if (fn->body) {
            collectFromStmt(fn->body.get());
        }
    } else if (decl->kind == ASTNodeKind::VarDecl) {
        const auto* vd = static_cast<const VarDeclNode*>(decl);
        if (vd->initializer) {
            collectFromExpr(vd->initializer.get());
        }
    }
}

void SyntaxHighlighter::collectFromStmt(const StmtNode* stmt) {
    if (!stmt) return;
    switch (stmt->kind) {
        case ASTNodeKind::Block: {
            const auto* blk = static_cast<const BlockNode*>(stmt);
            for (const auto& s : blk->statements) {
                collectFromStmt(s.get());
            }
            break;
        }
        case ASTNodeKind::IfStmt: {
            const auto* ifs = static_cast<const IfStmtNode*>(stmt);
            collectFromExpr(ifs->condition.get());
            collectFromStmt(ifs->thenBranch.get());
            collectFromStmt(ifs->elseBranch.get());
            break;
        }
        case ASTNodeKind::WhileStmt: {
            const auto* wh = static_cast<const WhileStmtNode*>(stmt);
            collectFromExpr(wh->condition.get());
            collectFromStmt(wh->body.get());
            break;
        }
        case ASTNodeKind::ForStmt: {
            const auto* fr = static_cast<const ForStmtNode*>(stmt);
            collectFromExpr(fr->initExpr.get());
            collectFromExpr(fr->condExpr.get());
            collectFromExpr(fr->updateExpr.get());
            collectFromStmt(fr->body.get());
            break;
        }
        case ASTNodeKind::ReturnStmt: {
            const auto* ret = static_cast<const ReturnStmtNode*>(stmt);
            collectFromExpr(ret->value.get());
            break;
        }
        case ASTNodeKind::ExprStmt: {
            const auto* es = static_cast<const ExprStmtNode*>(stmt);
            collectFromExpr(es->expr.get());
            break;
        }
        default:
            break;
    }
}

void SyntaxHighlighter::collectFromExpr(const ExprNode* expr) {
    if (!expr) return;
    switch (expr->kind) {
        case ASTNodeKind::CallExpr: {
            const auto* call = static_cast<const CallExprNode*>(expr);
            functionLocationKeys_.insert(makeLocKey(call->loc.line, call->loc.column));
            for (const auto& arg : call->args) {
                collectFromExpr(arg.get());
            }
            break;
        }
        case ASTNodeKind::BinaryExpr: {
            const auto* bin = static_cast<const BinaryExprNode*>(expr);
            collectFromExpr(bin->left.get());
            collectFromExpr(bin->right.get());
            break;
        }
        case ASTNodeKind::UnaryExpr: {
            const auto* un = static_cast<const UnaryExprNode*>(expr);
            collectFromExpr(un->operand.get());
            break;
        }
        case ASTNodeKind::AssignExpr: {
            const auto* asg = static_cast<const AssignExprNode*>(expr);
            collectFromExpr(asg->target.get());
            collectFromExpr(asg->value.get());
            break;
        }
        case ASTNodeKind::ArrayIndexExpr: {
            const auto* idx = static_cast<const ArrayIndexExprNode*>(expr);
            collectFromExpr(idx->arrayExpr.get());
            collectFromExpr(idx->indexExpr.get());
            break;
        }
        case ASTNodeKind::PostfixExpr: {
            const auto* post = static_cast<const PostfixExprNode*>(expr);
            collectFromExpr(post->operand.get());
            break;
        }
        default:
            break;
    }
}

HighlightCategory SyntaxHighlighter::classifyToken(const Token& tok) const {
    switch (tok.type) {
        case TokenType::KW_ARRAY:
        case TokenType::KW_ELSE:
        case TokenType::KW_FOR:
        case TokenType::KW_FUNCTION:
        case TokenType::KW_IF:
        case TokenType::KW_RETURN:
        case TokenType::KW_WHILE:
            return HighlightCategory::Keyword;

        case TokenType::KW_AUTO:
        case TokenType::KW_BOOLEAN:
        case TokenType::KW_CHAR:
        case TokenType::KW_FLOAT:
        case TokenType::KW_INTEGER:
        case TokenType::KW_STRING:
        case TokenType::KW_VOID:
            return HighlightCategory::TypeName;

        case TokenType::IDENTIFIER: {
            if (functionLocationKeys_.count(makeLocKey(tok.loc.line, tok.loc.column)) > 0) {
                return HighlightCategory::FunctionIdent;
            }
            return HighlightCategory::VariableIdent;
        }

        case TokenType::INT_LITERAL:
            return HighlightCategory::IntLiteral;
        case TokenType::FLOAT_LITERAL:
            return HighlightCategory::FloatLiteral;
        case TokenType::CHAR_LITERAL:
        case TokenType::STRING_LITERAL:
            return HighlightCategory::StringOrCharLiteral;
        case TokenType::KW_TRUE:
        case TokenType::KW_FALSE:
            return HighlightCategory::BoolLiteral;

        case TokenType::OP_PLUS_PLUS:
        case TokenType::OP_MINUS_MINUS:
        case TokenType::OP_LE:
        case TokenType::OP_GE:
        case TokenType::OP_EQ:
        case TokenType::OP_NE:
        case TokenType::OP_AND:
        case TokenType::OP_OR:
        case TokenType::OP_PLUS:
        case TokenType::OP_MINUS:
        case TokenType::OP_STAR:
        case TokenType::OP_SLASH:
        case TokenType::OP_MOD:
        case TokenType::OP_CARET:
        case TokenType::OP_ASSIGN:
        case TokenType::OP_LT:
        case TokenType::OP_GT:
        case TokenType::OP_NOT:
            return HighlightCategory::Operator;

        case TokenType::LPAREN:
        case TokenType::RPAREN:
        case TokenType::LBRACE:
        case TokenType::RBRACE:
        case TokenType::LBRACKET:
        case TokenType::RBRACKET:
        case TokenType::SEMI:
        case TokenType::COLON:
        case TokenType::COMMA:
            return HighlightCategory::Delimiter;

        case TokenType::INVALID:
            return HighlightCategory::ErrorToken;

        default:
            return HighlightCategory::Default;
    }
}

std::string SyntaxHighlighter::ansiWrap(HighlightCategory cat, const std::string& text) {
    const char* code = "";
    switch (cat) {
        case HighlightCategory::Keyword:             code = "\033[1;34m"; break; // Bold blue
        case HighlightCategory::TypeName:            code = "\033[36m";   break; // Cyan / Teal
        case HighlightCategory::VariableIdent:       code = "\033[37m";   break; // White
        case HighlightCategory::FunctionIdent:       code = "\033[1;33m"; break; // Yellow / Gold
        case HighlightCategory::IntLiteral:
        case HighlightCategory::FloatLiteral:
        case HighlightCategory::BoolLiteral:         code = "\033[38;5;208m"; break; // Orange
        case HighlightCategory::StringOrCharLiteral: code = "\033[32m";   break; // Green
        case HighlightCategory::Operator:            code = "\033[37m";   break; // Light gray
        case HighlightCategory::Comment:             code = "\033[3;90m"; break; // Dim gray italic
        case HighlightCategory::ErrorToken:          code = "\033[4;31m"; break; // Red underline
        default:                                     return text;
    }
    return std::string(code) + text + "\033[0m";
}

std::string SyntaxHighlighter::htmlClass(HighlightCategory cat) {
    switch (cat) {
        case HighlightCategory::Keyword:             return "kw";
        case HighlightCategory::TypeName:            return "type";
        case HighlightCategory::VariableIdent:       return "var";
        case HighlightCategory::FunctionIdent:       return "fn";
        case HighlightCategory::IntLiteral:
        case HighlightCategory::FloatLiteral:
        case HighlightCategory::BoolLiteral:         return "lit";
        case HighlightCategory::StringOrCharLiteral: return "str";
        case HighlightCategory::Operator:            return "op";
        case HighlightCategory::Delimiter:           return "delim";
        case HighlightCategory::Comment:             return "cmt";
        case HighlightCategory::ErrorToken:          return "err";
        default:                                     return "txt";
    }
}

std::string SyntaxHighlighter::escapeHtml(const std::string& text) {
    std::string out;
    out.reserve(text.size());
    for (char c : text) {
        switch (c) {
            case '&': out += "&amp;"; break;
            case '<': out += "&lt;"; break;
            case '>': out += "&gt;"; break;
            case '"': out += "&quot;"; break;
            default:  out.push_back(c); break;
        }
    }
    return out;
}

std::string SyntaxHighlighter::renderAnsi() const {
    std::ostringstream out;
    size_t idx = 0;
    size_t tokIdx = 0;
    int line = 1;
    int col = 1;

    while (idx < source_.size()) {
        // Skip whitespace preserving exact layout
        if (source_[idx] == ' ' || source_[idx] == '\t' || source_[idx] == '\r' || source_[idx] == '\n') {
            out << source_[idx];
            if (source_[idx] == '\n') { line++; col = 1; }
            else { col++; }
            idx++;
            continue;
        }

        // Single-line comment
        if (source_[idx] == '/' && idx + 1 < source_.size() && source_[idx + 1] == '/') {
            std::string cmt;
            while (idx < source_.size() && source_[idx] != '\n') {
                cmt.push_back(source_[idx++]);
                col++;
            }
            out << ansiWrap(HighlightCategory::Comment, cmt);
            continue;
        }

        // Block comment
        if (source_[idx] == '/' && idx + 1 < source_.size() && source_[idx + 1] == '*') {
            std::string cmt;
            cmt += "/*";
            idx += 2;
            col += 2;
            while (idx < source_.size()) {
                if (source_[idx] == '*' && idx + 1 < source_.size() && source_[idx + 1] == '/') {
                    cmt += "*/";
                    idx += 2;
                    col += 2;
                    break;
                }
                if (source_[idx] == '\n') { line++; col = 1; }
                else { col++; }
                cmt.push_back(source_[idx++]);
            }
            out << ansiWrap(HighlightCategory::Comment, cmt);
            continue;
        }

        while (tokIdx < tokens_.size() && tokens_[tokIdx].type == TokenType::END_OF_FILE) {
            tokIdx++;
        }

        if (tokIdx < tokens_.size() &&
            tokens_[tokIdx].loc.line == line &&
            tokens_[tokIdx].loc.column == col) {
            const Token& tok = tokens_[tokIdx++];
            HighlightCategory cat = classifyToken(tok);
            out << ansiWrap(cat, tok.lexeme);
            for (char c : tok.lexeme) {
                if (c == '\n') { line++; col = 1; }
                else { col++; }
                idx++;
            }
        } else {
            out << source_[idx];
            if (source_[idx] == '\n') { line++; col = 1; }
            else { col++; }
            idx++;
        }
    }
    return out.str();
}

std::string SyntaxHighlighter::renderHtml(const std::string& title) const {
    std::ostringstream out;
    out << "<!DOCTYPE html>\n<html lang=\"en\">\n<head>\n<meta charset=\"UTF-8\">\n"
        << "<title>" << escapeHtml(title) << "</title>\n"
        << "<style>\n"
        << "  body { background: #1e1e1e; color: #d4d4d4; font-family: Consolas, monospace; padding: 20px; }\n"
        << "  pre { background: #252526; padding: 16px; border-radius: 8px; line-height: 1.5; overflow-x: auto; }\n"
        << "  .kw { color: #569cd6; font-weight: bold; }\n"
        << "  .type { color: #4ec9b0; }\n"
        << "  .var { color: #d4d4d4; }\n"
        << "  .fn { color: #dcdcaa; font-weight: bold; }\n"
        << "  .lit { color: #ce9178; }\n"
        << "  .str { color: #6a9955; }\n"
        << "  .op { color: #b4b4b4; }\n"
        << "  .delim { color: #d4d4d4; }\n"
        << "  .cmt { color: #6a9955; font-style: italic; opacity: 0.75; }\n"
        << "  .err { color: #f44747; text-decoration: wavy underline #f44747; }\n"
        << "</style>\n</head>\n<body>\n<pre>";

    size_t idx = 0;
    size_t tokIdx = 0;
    int line = 1;
    int col = 1;

    while (idx < source_.size()) {
        if (source_[idx] == ' ' || source_[idx] == '\t' || source_[idx] == '\r' || source_[idx] == '\n') {
            out << source_[idx];
            if (source_[idx] == '\n') { line++; col = 1; }
            else { col++; }
            idx++;
            continue;
        }

        if (source_[idx] == '/' && idx + 1 < source_.size() && source_[idx + 1] == '/') {
            std::string cmt;
            while (idx < source_.size() && source_[idx] != '\n') {
                cmt.push_back(source_[idx++]);
                col++;
            }
            out << "<span class=\"cmt\">" << escapeHtml(cmt) << "</span>";
            continue;
        }

        if (source_[idx] == '/' && idx + 1 < source_.size() && source_[idx + 1] == '*') {
            std::string cmt;
            cmt += "/*";
            idx += 2;
            col += 2;
            while (idx < source_.size()) {
                if (source_[idx] == '*' && idx + 1 < source_.size() && source_[idx + 1] == '/') {
                    cmt += "*/";
                    idx += 2;
                    col += 2;
                    break;
                }
                if (source_[idx] == '\n') { line++; col = 1; }
                else { col++; }
                cmt.push_back(source_[idx++]);
            }
            out << "<span class=\"cmt\">" << escapeHtml(cmt) << "</span>";
            continue;
        }

        while (tokIdx < tokens_.size() && tokens_[tokIdx].type == TokenType::END_OF_FILE) {
            tokIdx++;
        }

        if (tokIdx < tokens_.size() &&
            tokens_[tokIdx].loc.line == line &&
            tokens_[tokIdx].loc.column == col) {
            const Token& tok = tokens_[tokIdx++];
            HighlightCategory cat = classifyToken(tok);
            out << "<span class=\"" << htmlClass(cat) << "\">" << escapeHtml(tok.lexeme) << "</span>";
            for (char c : tok.lexeme) {
                if (c == '\n') { line++; col = 1; }
                else { col++; }
                idx++;
            }
        } else {
            out << escapeHtml(std::string(1, source_[idx]));
            if (source_[idx] == '\n') { line++; col = 1; }
            else { col++; }
            idx++;
        }
    }

    out << "</pre>\n</body>\n</html>\n";
    return out.str();
}

} // namespace bminor
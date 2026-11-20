#ifndef BMINOR_HIGHLIGHTER_HPP
#define BMINOR_HIGHLIGHTER_HPP

#include "ast.hpp"
#include "token.hpp"
#include <string>
#include <unordered_set>
#include <vector>

namespace bminor {

enum class HighlightCategory {
    Default,
    Keyword,
    TypeName,
    VariableIdent,
    FunctionIdent,
    IntLiteral,
    FloatLiteral,
    StringOrCharLiteral,
    BoolLiteral,
    Operator,
    Delimiter,
    Comment,
    ErrorToken
};

class SyntaxHighlighter {
public:
    SyntaxHighlighter(std::string source,
                      std::vector<Token> tokens,
                      const ProgramNode* astRoot);

    std::string renderAnsi() const;
    std::string renderHtml(const std::string& title = "B-Minor Highlighted Source") const;

private:
    std::string source_;
    std::vector<Token> tokens_;
    std::unordered_set<std::string> functionLocationKeys_;

    static std::string makeLocKey(int line, int column);
    void collectFunctionLocations(const ProgramNode* root);
    void collectFromDecl(const DeclNode* decl);
    void collectFromStmt(const StmtNode* stmt);
    void collectFromExpr(const ExprNode* expr);

    HighlightCategory classifyToken(const Token& tok) const;
    static std::string ansiWrap(HighlightCategory cat, const std::string& text);
    static std::string htmlClass(HighlightCategory cat);
    static std::string escapeHtml(const std::string& text);
};

} // namespace bminor

#endif // BMINOR_HIGHLIGHTER_HPP
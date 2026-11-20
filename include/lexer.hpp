#ifndef BMINOR_LEXER_HPP
#define BMINOR_LEXER_HPP

#include "token.hpp"
#include <string>
#include <unordered_map>
#include <vector>

namespace bminor {

struct LexError {
    SourceLocation loc;
    std::string message;
};

class Lexer {
public:
    Lexer(std::string source, std::string filename = "<stdin>");

    std::vector<Token> tokenize();
    const std::vector<LexError>& errors() const { return errors_; }

private:
    std::string source_;
    std::string filename_;
    size_t pos_ = 0;
    int line_ = 1;
    int col_ = 1;

    std::vector<LexError> errors_;
    static const std::unordered_map<std::string, TokenType> keywords_;

    bool isAtEnd() const;
    char peek(size_t offset = 0) const;
    char advance();
    SourceLocation currentLocation() const;

    void skipWhitespaceAndComments();
    void reportError(const SourceLocation& loc, const std::string& msg);

    Token scanIdentifierOrKeyword();
    Token scanNumber();
    Token scanString();
    Token scanChar();
    Token scanOperatorOrDelimiter();
};

} // namespace bminor

#endif // BMINOR_LEXER_HPP
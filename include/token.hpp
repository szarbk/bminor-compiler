#ifndef BMINOR_TOKEN_HPP
#define BMINOR_TOKEN_HPP

#include <string>

namespace bminor {

struct SourceLocation {
    std::string filename;
    int line = 1;
    int column = 1;
};

enum class TokenType {
    // Keywords
    KW_ARRAY,
    KW_AUTO,
    KW_BOOLEAN,
    KW_CHAR,
    KW_ELSE,
    KW_FALSE,
    KW_FLOAT,
    KW_FOR,
    KW_FUNCTION,
    KW_IF,
    KW_INTEGER,
    KW_RETURN,
    KW_STRING,
    KW_TRUE,
    KW_VOID,
    KW_WHILE,

    // Identifiers & Literals
    IDENTIFIER,
    INT_LITERAL,
    FLOAT_LITERAL,
    CHAR_LITERAL,
    STRING_LITERAL,

    // Operators (multi-char & single-char)
    OP_PLUS_PLUS,   // ++
    OP_MINUS_MINUS, // --
    OP_LE,          // <=
    OP_GE,          // >=
    OP_EQ,          // ==
    OP_NE,          // !=
    OP_AND,         // &&
    OP_OR,          // ||
    OP_PLUS,        // +
    OP_MINUS,       // -
    OP_STAR,        // *
    OP_SLASH,       // /
    OP_MOD,         // %
    OP_CARET,       // ^
    OP_ASSIGN,      // =
    OP_LT,          // <
    OP_GT,          // >
    OP_NOT,         // !

    // Delimiters
    LPAREN,         // (
    RPAREN,         // )
    LBRACE,         // {
    RBRACE,         // }
    LBRACKET,       // [
    RBRACKET,       // ]
    SEMI,           // ;
    COLON,          // :
    COMMA,          // ,

    // Special / Error
    END_OF_FILE,
    INVALID
};

struct Token {
    TokenType type = TokenType::INVALID;
    std::string lexeme;
    SourceLocation loc;
};

std::string tokenTypeToString(TokenType type);

} // namespace bminor

#endif // BMINOR_TOKEN_HPP
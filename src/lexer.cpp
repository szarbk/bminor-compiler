#include "lexer.hpp"
#include <cctype>

namespace bminor {

const std::unordered_map<std::string, TokenType> Lexer::keywords_ = {
    {"array",    TokenType::KW_ARRAY},
    {"auto",     TokenType::KW_AUTO},
    {"boolean",  TokenType::KW_BOOLEAN},
    {"char",     TokenType::KW_CHAR},
    {"else",     TokenType::KW_ELSE},
    {"false",    TokenType::KW_FALSE},
    {"float",    TokenType::KW_FLOAT},
    {"for",      TokenType::KW_FOR},
    {"function", TokenType::KW_FUNCTION},
    {"if",       TokenType::KW_IF},
    {"integer",  TokenType::KW_INTEGER},
    {"return",   TokenType::KW_RETURN},
    {"string",   TokenType::KW_STRING},
    {"true",     TokenType::KW_TRUE},
    {"void",     TokenType::KW_VOID},
    {"while",    TokenType::KW_WHILE}
};

std::string tokenTypeToString(TokenType type) {
    switch (type) {
        case TokenType::KW_ARRAY: return "KW_ARRAY";
        case TokenType::KW_AUTO: return "KW_AUTO";
        case TokenType::KW_BOOLEAN: return "KW_BOOLEAN";
        case TokenType::KW_CHAR: return "KW_CHAR";
        case TokenType::KW_ELSE: return "KW_ELSE";
        case TokenType::KW_FALSE: return "KW_FALSE";
        case TokenType::KW_FLOAT: return "KW_FLOAT";
        case TokenType::KW_FOR: return "KW_FOR";
        case TokenType::KW_FUNCTION: return "KW_FUNCTION";
        case TokenType::KW_IF: return "KW_IF";
        case TokenType::KW_INTEGER: return "KW_INTEGER";
        case TokenType::KW_RETURN: return "KW_RETURN";
        case TokenType::KW_STRING: return "KW_STRING";
        case TokenType::KW_TRUE: return "KW_TRUE";
        case TokenType::KW_VOID: return "KW_VOID";
        case TokenType::KW_WHILE: return "KW_WHILE";
        case TokenType::IDENTIFIER: return "IDENTIFIER";
        case TokenType::INT_LITERAL: return "INT_LITERAL";
        case TokenType::FLOAT_LITERAL: return "FLOAT_LITERAL";
        case TokenType::CHAR_LITERAL: return "CHAR_LITERAL";
        case TokenType::STRING_LITERAL: return "STRING_LITERAL";
        case TokenType::OP_PLUS_PLUS: return "OP_PLUS_PLUS";
        case TokenType::OP_MINUS_MINUS: return "OP_MINUS_MINUS";
        case TokenType::OP_LE: return "OP_LE";
        case TokenType::OP_GE: return "OP_GE";
        case TokenType::OP_EQ: return "OP_EQ";
        case TokenType::OP_NE: return "OP_NE";
        case TokenType::OP_AND: return "OP_AND";
        case TokenType::OP_OR: return "OP_OR";
        case TokenType::OP_PLUS: return "OP_PLUS";
        case TokenType::OP_MINUS: return "OP_MINUS";
        case TokenType::OP_STAR: return "OP_STAR";
        case TokenType::OP_SLASH: return "OP_SLASH";
        case TokenType::OP_MOD: return "OP_MOD";
        case TokenType::OP_CARET: return "OP_CARET";
        case TokenType::OP_ASSIGN: return "OP_ASSIGN";
        case TokenType::OP_LT: return "OP_LT";
        case TokenType::OP_GT: return "OP_GT";
        case TokenType::OP_NOT: return "OP_NOT";
        case TokenType::LPAREN: return "LPAREN";
        case TokenType::RPAREN: return "RPAREN";
        case TokenType::LBRACE: return "LBRACE";
        case TokenType::RBRACE: return "RBRACE";
        case TokenType::LBRACKET: return "LBRACKET";
        case TokenType::RBRACKET: return "RBRACKET";
        case TokenType::SEMI: return "SEMI";
        case TokenType::COLON: return "COLON";
        case TokenType::COMMA: return "COMMA";
        case TokenType::END_OF_FILE: return "EOF";
        case TokenType::INVALID: return "INVALID";
    }
    return "UNKNOWN";
}

Lexer::Lexer(std::string source, std::string filename)
    : source_(std::move(source)), filename_(std::move(filename)) {}

bool Lexer::isAtEnd() const {
    return pos_ >= source_.size();
}

char Lexer::peek(size_t offset) const {
    if (pos_ + offset >= source_.size()) return '\0';
    return source_[pos_ + offset];
}

char Lexer::advance() {
    if (isAtEnd()) return '\0';
    char c = source_[pos_++];
    if (c == '\n') {
        line_++;
        col_ = 1;
    } else {
        col_++;
    }
    return c;
}

SourceLocation Lexer::currentLocation() const {
    return SourceLocation{filename_, line_, col_};
}

void Lexer::reportError(const SourceLocation& loc, const std::string& msg) {
    errors_.push_back(LexError{loc, msg});
}

void Lexer::skipWhitespaceAndComments() {
    while (!isAtEnd()) {
        char c = peek();
        if (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
            advance();
        } else if (c == '/' && peek(1) == '/') {
            advance();
            advance();
            while (!isAtEnd() && peek() != '\n') {
                advance();
            }
        } else if (c == '/' && peek(1) == '*') {
            SourceLocation startLoc = currentLocation();
            advance();
            advance();
            bool closed = false;
            while (!isAtEnd()) {
                if (peek() == '*' && peek(1) == '/') {
                    advance();
                    advance();
                    closed = true;
                    break;
                }
                advance();
            }
            if (!closed) {
                reportError(startLoc, "Unterminated block comment");
            }
        } else {
            break;
        }
    }
}

Token Lexer::scanIdentifierOrKeyword() {
    SourceLocation startLoc = currentLocation();
    std::string text;
    while (!isAtEnd() && (std::isalnum(static_cast<unsigned char>(peek())) || peek() == '_')) {
        text.push_back(advance());
    }
    // Priority: Keywords are checked before Identifiers
    auto it = keywords_.find(text);
    if (it != keywords_.end()) {
        return Token{it->second, text, startLoc};
    }
    return Token{TokenType::IDENTIFIER, text, startLoc};
}

Token Lexer::scanNumber() {
    SourceLocation startLoc = currentLocation();
    std::string text;
    while (!isAtEnd() && std::isdigit(static_cast<unsigned char>(peek()))) {
        text.push_back(advance());
    }
    // Longest match for float literal when a fractional digit follows '.'
    if (peek() == '.' && std::isdigit(static_cast<unsigned char>(peek(1)))) {
        text.push_back(advance());
        while (!isAtEnd() && std::isdigit(static_cast<unsigned char>(peek()))) {
            text.push_back(advance());
        }
        return Token{TokenType::FLOAT_LITERAL, text, startLoc};
    }
    return Token{TokenType::INT_LITERAL, text, startLoc};
}

Token Lexer::scanString() {
    SourceLocation startLoc = currentLocation();
    std::string text;
    text.push_back(advance()); // opening '"'
    bool closed = false;

    while (!isAtEnd()) {
        char c = peek();
        if (c == '"') {
            text.push_back(advance());
            closed = true;
            break;
        }
        if (c == '\\') {
            text.push_back(advance());
            if (!isAtEnd()) {
                text.push_back(advance());
            }
        } else {
            text.push_back(advance());
        }
    }

    if (!closed) {
        reportError(startLoc, "Unterminated string literal");
        return Token{TokenType::INVALID, text, startLoc};
    }
    return Token{TokenType::STRING_LITERAL, text, startLoc};
}

Token Lexer::scanChar() {
    SourceLocation startLoc = currentLocation();
    std::string text;
    text.push_back(advance()); // opening '\''
    bool closed = false;

    while (!isAtEnd()) {
        char c = peek();
        if (c == '\'') {
            text.push_back(advance());
            closed = true;
            break;
        }
        if (c == '\\') {
            text.push_back(advance());
            if (!isAtEnd()) {
                text.push_back(advance());
            }
        } else {
            text.push_back(advance());
        }
    }

    if (!closed) {
        reportError(startLoc, "Unterminated character literal");
        return Token{TokenType::INVALID, text, startLoc};
    }
    return Token{TokenType::CHAR_LITERAL, text, startLoc};
}

Token Lexer::scanOperatorOrDelimiter() {
    SourceLocation loc = currentLocation();
    char c = peek();
    char next = peek(1);

    // Longest match for two-character operators
    if (c == '+' && next == '+') { advance(); advance(); return Token{TokenType::OP_PLUS_PLUS, "++", loc}; }
    if (c == '-' && next == '-') { advance(); advance(); return Token{TokenType::OP_MINUS_MINUS, "--", loc}; }
    if (c == '<' && next == '=') { advance(); advance(); return Token{TokenType::OP_LE, "<=", loc}; }
    if (c == '>' && next == '=') { advance(); advance(); return Token{TokenType::OP_GE, ">=", loc}; }
    if (c == '=' && next == '=') { advance(); advance(); return Token{TokenType::OP_EQ, "==", loc}; }
    if (c == '!' && next == '=') { advance(); advance(); return Token{TokenType::OP_NE, "!=", loc}; }
    if (c == '&' && next == '&') { advance(); advance(); return Token{TokenType::OP_AND, "&&", loc}; }
    if (c == '|' && next == '|') { advance(); advance(); return Token{TokenType::OP_OR, "||", loc}; }

    advance();
    std::string s(1, c);

    switch (c) {
        case '+': return Token{TokenType::OP_PLUS, s, loc};
        case '-': return Token{TokenType::OP_MINUS, s, loc};
        case '*': return Token{TokenType::OP_STAR, s, loc};
        case '/': return Token{TokenType::OP_SLASH, s, loc};
        case '%': return Token{TokenType::OP_MOD, s, loc};
        case '^': return Token{TokenType::OP_CARET, s, loc};
        case '=': return Token{TokenType::OP_ASSIGN, s, loc};
        case '<': return Token{TokenType::OP_LT, s, loc};
        case '>': return Token{TokenType::OP_GT, s, loc};
        case '!': return Token{TokenType::OP_NOT, s, loc};
        case '(': return Token{TokenType::LPAREN, s, loc};
        case ')': return Token{TokenType::RPAREN, s, loc};
        case '{': return Token{TokenType::LBRACE, s, loc};
        case '}': return Token{TokenType::RBRACE, s, loc};
        case '[': return Token{TokenType::LBRACKET, s, loc};
        case ']': return Token{TokenType::RBRACKET, s, loc};
        case ';': return Token{TokenType::SEMI, s, loc};
        case ':': return Token{TokenType::COLON, s, loc};
        case ',': return Token{TokenType::COMMA, s, loc};
        default:
            reportError(loc, "Unexpected character '" + s + "'");
            return Token{TokenType::INVALID, s, loc};
    }
}

std::vector<Token> Lexer::tokenize() {
    std::vector<Token> tokens;
    while (true) {
        skipWhitespaceAndComments();
        if (isAtEnd()) {
            tokens.push_back(Token{TokenType::END_OF_FILE, "", currentLocation()});
            break;
        }
        char c = peek();
        if (std::isalpha(static_cast<unsigned char>(c)) || c == '_') {
            tokens.push_back(scanIdentifierOrKeyword());
        } else if (std::isdigit(static_cast<unsigned char>(c))) {
            tokens.push_back(scanNumber());
        } else if (c == '"') {
            tokens.push_back(scanString());
        } else if (c == '\'') {
            tokens.push_back(scanChar());
        } else {
            tokens.push_back(scanOperatorOrDelimiter());
        }
    }
    return tokens;
}

} // namespace bminor
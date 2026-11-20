# \# B-Minor Compiler — Phase 1 (Lexer, Parser, AST \& Syntax Highlighter)

# 

# This repository contains the Phase 1 implementation of a front-end and AST-driven syntax highlighter for the \*\*B-Minor\*\* programming language, written in standard \*\*C++\*\*.

# 

# \## Project Structure

# 

# \- `docs/bminor\_grammar.ebnf`: Lexical token rules, precedence/matching policies, and left-recursion-free EBNF syntactic grammar for B-Minor.

# \- `include/token.hpp`, `include/lexer.hpp`, `src/lexer.cpp`: Lexical analyzer with line/column tracking, longest-match token selection, keyword-over-identifier priority, and non-fatal error recovery (`INVALID` tokens and unterminated string/comment reporting).

# \- `include/ast.hpp`: Abstract Syntax Tree (AST) node definitions with `SourceLocation` tracking and an optional `typeAnnotation` field reserved for semantic analysis in later phases.

# \- `include/parser.hpp`, `src/parser.cpp`: Recursive-descent (top-down) parser building the AST with panic-mode error recovery (`;`, `}`, and control keywords as synchronization points).

# \- `include/highlighter.hpp`, `src/highlighter.cpp`: AST-assisted syntax highlighter (distinguishing function declarations/calls from variable identifiers) generating both ANSI terminal output and standalone HTML/CSS (`<pre>` + `<span>`).

# \- `src/main.cpp`: Command-line driver that runs lexing, parsing, AST printing, error reporting, and ANSI/HTML rendering.

# \- `tests/valid\_sample.bminor`, `tests/invalid\_sample.bminor`: Sample valid and invalid B-Minor programs.

# 

# \## Build \& Run (C++)

# 

# Using `g++` (C++17 or later):

# 

# ```bash

# g++ -std=c++17 -Wall -Wextra -Iinclude src/lexer.cpp src/parser.cpp src/highlighter.cpp src/main.cpp -o bminor\_front


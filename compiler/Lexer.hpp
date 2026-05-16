#ifndef QUELLC_LEXER_HPP
#define QUELLC_LEXER_HPP

#include "../datastructs/DynamicArray.hpp"
#include "../datastructs/String.hpp"
#include "Token.hpp"

struct LexResult {
    bool ok = true;
    DynamicArray<String> errors; // for all errors
    DynamicArray<Token> tokens; // for all tokens
};

class Lexer {
    public:
        explicit Lexer(const String& source); // construct
        LexResult lex(); // return tokens

    private:
        const String& src; // source file
        size_t pos; // pos in file

        bool eof() const; // at end of file
        char peek(size_t off = 0) const; // peek next char
        char get(); // return and remove next char

        void skip_ws_and_comments(); // duh

        static bool is_ident_start(char c); // start of identifier
        static bool is_ident_continue(char c); // in middle of identifier

        Token make_from_slice(TokenKind k, size_t begin, size_t end) const; // construct from slice
        Token make(TokenKind k, size_t begin, size_t end, const char* text) const; // construct normally

        TokenKind keyword_kind(const String& s) const; // convert string to tokenkind

        Token lex_identifier_or_keyword(); // not number
        Token lex_number(); // number (duh)
};

#endif

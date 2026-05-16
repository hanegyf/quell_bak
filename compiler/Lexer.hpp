#ifndef QUELLC_LEXER_HPP
#define QUELLC_LEXER_HPP

#include "../datastructs/DynamicArray.hpp"
#include "../datastructs/String.hpp"
#include "Token.hpp"

struct LexResult {
    bool ok = true;
    DynamicArray<String> errors;
    DynamicArray<Token> tokens;
};

class Lexer {
    public:
        explicit Lexer(const String& source);
        LexResult lex();

    private:
        const String& src;
        size_t pos;

        bool eof() const;
        char peek(size_t off = 0) const;
        char get();

        void skip_ws_and_comments();

        static bool is_ident_start(char c);
        static bool is_ident_continue(char c);

        Token make_from_slice(TokenKind k, size_t begin, size_t end) const;
        Token make(TokenKind k, size_t begin, size_t end, const char* text) const;

        TokenKind keyword_kind(const String& s) const;

        Token lex_identifier_or_keyword();
        Token lex_number();
};

#endif

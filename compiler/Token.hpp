#ifndef QUELLC_TOKEN_HPP
#define QUELLC_TOKEN_HPP

#include "../datastructs/String.hpp"
#include <cstddef>

enum class TokenKind {
    End,

    Identifier,
    Number,

    // Keywords
    KwFunc,
    KwLet,

    // type Keywords
    KwVoid,
    KwQreg,
    KwBreg,

    // gate keywords (reserved)
    KwH, KwX, KwY, KwZ, KwS, KwT,
    KwCX, KwCZ, KwCNOT, KwSWAP, // both CX and CNOT valid
    KwMEASURE, KwRESET,
    
    // Punctuation
    LParen, RParen,
    LBrace, RBrace,
    LBracket, RBracket,
    Comma, Colon, Semicolon,

    // other stuff
    Arrow, // ->
    Lt, Gt, // < >
};

struct Span { // tells length of a token
    size_t begin = 0;
    size_t end = 0; // exclusive
};

struct Token {
    TokenKind kind = TokenKind::End; // default to end
    String lexeme; // actual lexeme of the token
    Span span; // span of the token
};

#endif

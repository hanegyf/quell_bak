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

    // Classical control
    KwIf,
    KwElse,
    KwTrue,
    KwFalse,

    // type Keywords
    KwVoid,
    KwQreg,
    KwBreg,

    // Classical primitive types
    KwInt,
    KwBool,

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
    
    // Operators and punctuation, ig (i don't know what to call it)
    Assign,     // =
    Plus,       // +
    Minus,      // -
    Star,       // *
    Slash,      // /

    EqEq,       // ==
    NotEq,      // !=
    LtEq,       // <=
    GtEq        // >=
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

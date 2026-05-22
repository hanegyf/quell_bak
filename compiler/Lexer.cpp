#include "Lexer.hpp"
#include <cctype>
#include <cstdlib>

Lexer::Lexer(const String& source) : src(source), pos(0) {} // default lexer

bool Lexer::eof() const { return pos >= src.size(); } // if at end

char Lexer::peek(size_t off) const { // grab next character
    size_t i = pos + off;
    if (i >= src.size()) return '\0';
    return src.c_str()[i];
}

char Lexer::get() { // consume next character
    if (eof()) return '\0';
    return src.c_str()[pos++];
}

bool Lexer::is_ident_start(char c) {
    return std::isalpha(static_cast<unsigned char>(c)) || c == '_';
}

bool Lexer::is_ident_continue(char c) {
    return std::isalnum(static_cast<unsigned char>(c)) || c == '_';
}

void Lexer::skip_ws_and_comments() {
    while (!eof()) {
        while (!eof() && std::isspace(static_cast<unsigned char>(peek())))
            get();

        if (peek() == '#') {
            while (!eof() && peek() != '\n') get();
            continue;
        }

        break;
    }
}

Token Lexer::make(TokenKind k, size_t begin, size_t end, const char* text) const {
    Token t;
    t.kind = k;
    t.span.begin = begin;
    t.span.end = end;
    t.lexeme = String(text);
    return t;
}

Token Lexer::make_from_slice(TokenKind k, size_t begin, size_t end) const {
    DynamicArray<char> buf;
    for (size_t i = begin; i < end; ++i) buf.push_back(src.c_str()[i]);
    buf.push_back('\0');

    Token t;
    t.kind = k;
    t.span.begin = begin;
    t.span.end = end;
    t.lexeme = String(&buf[0]);
    return t;
}

TokenKind Lexer::keyword_kind(const String& s) const {
  if (s == "func") return TokenKind::KwFunc;
  if (s == "let") return TokenKind::KwLet;

  if (s == "if") return TokenKind::KwIf;
  if (s == "else") return TokenKind::KwElse;
  if (s == "true") return TokenKind::KwTrue;
  if (s == "false") return TokenKind::KwFalse;
  if (s == "return") return TokenKind::KwReturn;

  if (s == "void") return TokenKind::KwVoid;
  if (s == "qreg") return TokenKind::KwQreg;
  if (s == "breg") return TokenKind::KwBreg;

  if (s == "int") return TokenKind::KwInt;
  if (s == "bool") return TokenKind::KwBool;

  // gates
  if (s == "H") return TokenKind::KwH;
  if (s == "X") return TokenKind::KwX;
  if (s == "Y") return TokenKind::KwY;
  if (s == "Z") return TokenKind::KwZ;
  if (s == "S") return TokenKind::KwS;
  if (s == "T") return TokenKind::KwT;

  // Parameterized gates
  if (s == "RX") return TokenKind::KwRX;
  if (s == "RY") return TokenKind::KwRY;
  if (s == "RZ") return TokenKind::KwRZ;
  if (s == "PHASE") return TokenKind::KwPHASE;

  if (s == "CX") return TokenKind::KwCX;
  if (s == "CZ") return TokenKind::KwCZ;
  if (s == "CNOT") return TokenKind::KwCNOT;
  if (s == "SWAP") return TokenKind::KwSWAP;
  if (s == "MEASURE") return TokenKind::KwMEASURE;
  if (s == "RESET") return TokenKind::KwRESET;

  return TokenKind::Identifier;
}

Token Lexer::lex_identifier_or_keyword() {
    size_t begin = pos;
    get();
    while (!eof() && is_ident_continue(peek())) get();
    Token t = make_from_slice(TokenKind::Identifier, begin, pos);
    t.kind = keyword_kind(t.lexeme);
    return t;
}

Token Lexer::lex_number() {
    size_t begin = pos;
    while (!eof() && std::isdigit(static_cast<unsigned char>(peek()))) get();
    return make_from_slice(TokenKind::Number, begin, pos);
}

LexResult Lexer::lex() {
    LexResult out;

    while (!eof()) {
        skip_ws_and_comments();
        if (eof()) break;

        size_t begin = pos;
        char c = peek();

        if (is_ident_start(c)) { out.tokens.push_back(lex_identifier_or_keyword()); continue; }
        if (std::isdigit(static_cast<unsigned char>(c))) { out.tokens.push_back(lex_number()); continue; }

        if (c == '-' && peek(1) == '>') { get(); get(); out.tokens.push_back(make(TokenKind::Arrow,  begin, pos, "->")); continue; }

        if (c == '=' && peek(1) == '=') { get(); get(); out.tokens.push_back(make(TokenKind::EqEq, begin, pos, "==")); continue; }
        if (c == '!' && peek(1) == '=') { get(); get(); out.tokens.push_back(make(TokenKind::NotEq, begin, pos, "!=")); continue; }
        if (c == '<' && peek(1) == '=') { get(); get(); out.tokens.push_back(make(TokenKind::LtEq, begin, pos, "<=")); continue; }
        if (c == '>' && peek(1) == '=') { get(); get(); out.tokens.push_back(make(TokenKind::GtEq, begin, pos, ">=")); continue; }

        switch (c) {
            case '(': get(); out.tokens.push_back(make(TokenKind::LParen, begin, pos, "(")); continue;
            case ')': get(); out.tokens.push_back(make(TokenKind::RParen, begin, pos, ")")); continue;
            case '{': get(); out.tokens.push_back(make(TokenKind::LBrace, begin, pos, "{")); continue;
            case '}': get(); out.tokens.push_back(make(TokenKind::RBrace, begin, pos, "}")); continue;
            case '[': get(); out.tokens.push_back(make(TokenKind::LBracket, begin, pos, "[")); continue;
            case ']': get(); out.tokens.push_back(make(TokenKind::RBracket, begin, pos, "]")); continue;
            case ',': get(); out.tokens.push_back(make(TokenKind::Comma, begin, pos, ",")); continue;
            case ':': get(); out.tokens.push_back(make(TokenKind::Colon, begin, pos, ":")); continue;
            case ';': get(); out.tokens.push_back(make(TokenKind::Semicolon, begin, pos, ";")); continue;
            case '<': get(); out.tokens.push_back(make(TokenKind::Lt, begin, pos, "<")); continue;
            case '>': get(); out.tokens.push_back(make(TokenKind::Gt, begin, pos, ">")); continue;

            case '=': get(); out.tokens.push_back(make(TokenKind::Assign, begin, pos, "=")); continue;
            case '+': get(); out.tokens.push_back(make(TokenKind::Plus, begin, pos, "+")); continue;
            case '-': get(); out.tokens.push_back(make(TokenKind::Minus, begin, pos, "-")); continue;
            case '*': get(); out.tokens.push_back(make(TokenKind::Star, begin, pos, "*")); continue;
            case '/': get(); out.tokens.push_back(make(TokenKind::Slash, begin, pos, "/")); continue;

            default:
                out.ok = false;
                out.errors.push_back(String("Unexpected characer in lexer"));
                get();
                continue;
        }
    }

    Token end;
    end.kind = TokenKind::End;
    end.lexeme = String("");
    end.span.begin = pos;
    end.span.end = pos;
    out.tokens.push_back(end);

    return out;
}

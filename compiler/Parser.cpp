#include "Parser.hpp"
#include <cstdlib>

Parser::Parser(const DynamicArray<Token> &toks) : t(toks), i(0), errors(nullptr), ok(true) {} // constructor

const Token& Parser::cur() const { return t[i]; } // current token
bool Parser::at(TokenKind k) const {return cur().kind == k; } // if where we are is that token kind

bool Parser::match(TokenKind k) { // match and move on
    if (at(k)) { ++i; return true; }
    return false;
}

void Parser::error(const char* msg) { // error message handling
    ok = false;
    if (errors) errors->push_back(String(msg));
}

bool Parser::expect(TokenKind k, const char* msg) { // expect a character, consume it, if doesn't work raise an error
    if (match(k)) return true;
    error(msg);
    return false;
}

ParseResult Parser::parse_program() { // parse the entire program
    ParseResult r;
    errors = &r.errors;
    ok = true;

    while (!at(TokenKind::End)) {
        Function fn;
        if (!parse_function(fn)) { // each element is a function
            r.ok = false;
            return r;
        }
        r.program.functions.push_back(fn);
    }

    r.ok = ok;
    return r;
}

bool Parser::parse_function(Function& out) { // parse a function: func name() -> void {...}
    if (!expect(TokenKind::KwFunc, "Expected 'func'")) return false;
    if (!expect(TokenKind::Identifier, "Expected function name")) return false;
    out.name = t[i-1].lexeme;

    // TODO: make it handle parameters
    if (!expect(TokenKind::LParen, "Expected '(' after function name")) return false;
    if (!expect(TokenKind::RParen, "Expected')' (params not handled yet)")) return false;

    if (!expect(TokenKind::Arrow, "Expected '->'")) return false;
    if (!expect(TokenKind::KwVoid, "Expected return type 'void' (returning not handled yet)")) return false;

    return parse_block(out);
}

bool Parser::parse_block(Function& fn) { // parse a block {...}
    if (!expect(TokenKind::LBrace, "Expected '{' to start function body")) return false;
    
    while (!at(TokenKind::RBrace) && !at(TokenKind::End)) {
        Stmt s;
        if (!parse_stmt(s)) return false;
        fn.body.push_back(s);
    }

    return expect(TokenKind::RBrace, "Expected '}' to end function body");
}

bool Parser::parse_stmt(Stmt& out) { // parse an individual statement
    if (match(TokenKind::Semicolon)) {
        out.kind = Stmt::Kind::Empty;
        return true;
    }

    if (at(TokenKind::KwLet)) return parse_let_reg(out);

    switch (cur().kind) { // handles pretty much only gates right now - omg this takes so long to write
        case TokenKind::KwH: case TokenKind::KwX: case TokenKind::KwY: case TokenKind::KwZ: case TokenKind::KwS: case TokenKind::KwT: case TokenKind::KwCX: case TokenKind::KwCZ: case TokenKind::KwCNOT: case TokenKind::KwSWAP: case TokenKind::KwMEASURE: case TokenKind::KwRESET:
            return parse_gate_stmt(out);
        default: // TODO: handle other types of statements
            error("Unsopported statement (expected let or gate stmt)");
            return false;
    }
}

bool Parser::parse_reg_type_size(size_t& outSize, bool& isQReg) { // parse a register declaration
    if (match(TokenKind::KwQreg)) isQReg = true;
    else if (match(TokenKind::KwBreg)) isQReg = false;
    else {error("Expected qreg or breg"); return false; }

    if (!expect(TokenKind::Lt, "Expected '<' in register type")) return false;
    if (!expect(TokenKind::Number, "Expected integer size in register type")) return false;
    
    outSize = static_cast<size_t>(std::strtoull(t[i-1].lexeme.c_str(), nullptr, 10));

    if (!expect(TokenKind::Gt, "Expected '>' in register type")) return false;
    return true;
}

bool Parser::parse_let_reg(Stmt& out) { // parse a let statement - only allows registers so far (TODO: handle non registers) (let name: qreg<size>; or let name: breg<size>)
    expect(TokenKind::KwLet, "Expected let");

    if (!expect(TokenKind::Identifier, "Expected identifier after let")) return false;
    String name = t[i-1].lexeme;

    if (!expect(TokenKind::Colon, "Expected ':' after let name")) return false;

    size_t size = 0;
    bool isQ = false;
    if (!parse_reg_type_size(size, isQ)) return false;

    if(!expect(TokenKind::Semicolon, "Expected ';' after let declaration")) return false;

    out.kind = Stmt::Kind::LetReg;
    out.letreg.name = name;
    out.letreg.size = size;
    out.letreg.kind = isQ ? LetRegDecl::Kind::QReg : LetRegDecl::Kind::BReg;
    return true;
}

bool Parser::parse_operand(Operand& out) { // parse an operand (identifier[index]) - doesn't support other operands yet (TODO: support more operands)
    if (!expect(TokenKind::Identifier, "Expected operand base identifier")) return false;
    out.base = t[i-1].lexeme;
    
    if (!expect(TokenKind::LBracket, "Expected '[' in operand")) return false;
    if (!expect(TokenKind::Number, "Expected integer index in operand")) return false;

    out.index = static_cast<size_t>(std::strtoull(t[i-1].lexeme.c_str(), nullptr, 10));

    if (!expect(TokenKind::RBracket, "Expected ']' in operand")) return false;

  return true;
}

bool Parser::parse_gate_stmt(Stmt& out) { // parse a gate statement ( gate[int][int][int]...;)
    TokenKind tk = cur().kind;
    ++i;

    GateStmtKind gk;
    if (tk == TokenKind::KwH) gk = GateStmtKind::H;
    else if (tk == TokenKind::KwX) gk = GateStmtKind::X;
    else if (tk == TokenKind::KwY) gk = GateStmtKind::Y;
    else if (tk == TokenKind::KwZ) gk = GateStmtKind::Z;
    else if (tk == TokenKind::KwS) gk = GateStmtKind::S;
    else if (tk == TokenKind::KwT) gk = GateStmtKind::T;
    else if (tk == TokenKind::KwCX) gk = GateStmtKind::CX;
    else if (tk == TokenKind::KwCZ) gk = GateStmtKind::CZ;
    else if (tk == TokenKind::KwCNOT) gk = GateStmtKind::CNOT;
    else if (tk == TokenKind::KwSWAP) gk = GateStmtKind::SWAP;
    else if (tk == TokenKind::KwMEASURE) gk = GateStmtKind::MEASURE;
    else gk = GateStmtKind::RESET;

    GateStmt gs;
    gs.kind = gk;

    // Parse operands until ';'
    while (!at(TokenKind::Semicolon)) {
        Operand op;
        if (!parse_operand(op)) return false;
        gs.ops.push_back(op);
    }
    expect(TokenKind::Semicolon, "Expected ';' after gate stmt");

    out.kind = Stmt::Kind::Gate;
    out.gate = gs;
    return true;
}

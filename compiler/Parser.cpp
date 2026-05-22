#include "Parser.hpp"
#include <cstdlib>
#include <cstdio>

Parser::Parser(const DynamicArray<Token> &toks) : t(toks), i(0), errors(nullptr), ok(true), current_fn(nullptr) {} // constructor

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

bool Parser::parse_type_ref(TypeRef& out) { // parse a type
    // void | int | bool | qreg<Number> | breg<Number>
    if (match(TokenKind::KwVoid)) { out.kind = TypeKind::Void; out.reg_size = 0; return true; }
    if (match(TokenKind::KwInt))  { out.kind = TypeKind::Int;  out.reg_size = 0; return true; }
    if (match(TokenKind::KwBool)) { out.kind = TypeKind::Bool; out.reg_size = 0; return true; }

    if (match(TokenKind::KwQreg) || match(TokenKind::KwBreg)) {
        bool isQ = (t[i-1].kind == TokenKind::KwQreg);
        if (!expect(TokenKind::Lt, "Expected '<' in register type")) return false;
        if (!expect(TokenKind::Number, "Expected integer size in register type")) return false;
        size_t sz = static_cast<size_t>(std::strtoull(t[i-1].lexeme.c_str(), nullptr, 10));
        if (!expect(TokenKind::Gt, "Expected '>' in register type")) return false;

        out.kind = isQ ? TypeKind::QReg : TypeKind::BReg;
        out.reg_size = sz;
        return true;
    }

    error("Expected type");
    return false;
}

bool Parser::parse_param_list(DynamicArray<Param>& out_params) { // parse a list of parameters
    // empty or: ident : type ( , ident : type )*
    if (at(TokenKind::RParen)) return true;

    while (true) {
        if (!expect(TokenKind::Identifier, "Expected parameter name")) return false;
        Param p;
        p.name = t[i-1].lexeme;

        if (!expect(TokenKind::Colon, "Expected ':' after parameter name")) return false;
        if (!parse_type_ref(p.type)) return false;

        out_params.push_back(p);

        if (match(TokenKind::Comma)) continue;
        break;
    }

    return true;
}

bool Parser::parse_function(Function& out) { // parse a function: func name(params) -> type {...}
    if (!expect(TokenKind::KwFunc, "Expected 'func'")) return false;
    if (!expect(TokenKind::Identifier, "Expected function name")) return false;
    out.name = t[i-1].lexeme;

    if (!expect(TokenKind::LParen, "Expected '(' after function name")) return false;
    if (!parse_param_list(out.params)) return false;
    if (!expect(TokenKind::RParen, "Expected ')' after parameters")) return false;

    if (!expect(TokenKind::Arrow, "Expected '->'")) return false;

    if (!parse_type_ref(out.return_type)) return false;

    current_fn = &out;
    bool ok_block = parse_block(out, out.body);
    current_fn = nullptr;
    return ok_block;
}

bool Parser::parse_block(Function& fn, DynamicArray<Stmt>& out_body) { // parse a block {...}
    if (!expect(TokenKind::LBrace, "Expected '{' to start block")) return false;

    while (!at(TokenKind::RBrace) && !at(TokenKind::End)) {
        Stmt s;
        if (!parse_stmt(s)) return false;
        out_body.push_back(s);
    }

    return expect(TokenKind::RBrace, "Expected '}' to end block");
}

bool Parser::parse_stmt(Stmt& out) { // parse an individual statement
    if (match(TokenKind::Semicolon)) {
        out.kind = Stmt::Kind::Empty;
        return true;
    }

    if (at(TokenKind::KwLet)) {
        size_t save = i;
        Stmt tmp;
        if (parse_let_reg(tmp)) { out = tmp; return true; }
        i = save;
        return parse_let_scalar(out);
    }

    if (at(TokenKind::KwIf)) return parse_if(out);
    if (at(TokenKind::KwReturn)) return parse_return(out);

    switch (cur().kind) {
        case TokenKind::KwH: case TokenKind::KwX: case TokenKind::KwY: case TokenKind::KwZ:
        case TokenKind::KwS: case TokenKind::KwT:
        case TokenKind::KwRX: case TokenKind::KwRY: case TokenKind::KwRZ: case TokenKind::KwPHASE:
        case TokenKind::KwCX: case TokenKind::KwCZ: case TokenKind::KwCNOT:
        case TokenKind::KwSWAP:
        case TokenKind::KwMEASURE:
        case TokenKind::KwRESET:
            return parse_gate_stmt(out);
        default:
            error("Unsopported statement");
            return false;
    }
}

bool Parser::parse_return(Stmt& out) {
    expect(TokenKind::KwReturn, "Expected return");

    if (match(TokenKind::Semicolon)) {
        out.kind = Stmt::Kind::Return;
        out.ret_expr = nullptr;
        return true;
    }

    Expr* e = parse_expr();
    if (!e) return false;
    if (!expect(TokenKind::Semicolon, "Expected ';' after return")) return false;

    out.kind = Stmt::Kind::Return;
    out.ret_expr = e;
    return true;
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

bool Parser::parse_let_reg(Stmt& out) {
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

bool Parser::parse_let_scalar(Stmt& out) {
    expect(TokenKind::KwLet, "Expected let");

    if (!expect(TokenKind::Identifier, "Expected identifier after let")) return false;
    String name = t[i-1].lexeme;

    if (!expect(TokenKind::Colon, "Expected ':' after let name")) return false;

    ScalarType ty;
    if (match(TokenKind::KwInt)) ty = ScalarType::Int;
    else if (match(TokenKind::KwBool)) ty = ScalarType::Bool;
    else { error("Expected int or bool type"); return false; }

    if (!expect(TokenKind::Assign, "Expected '=' in scalar let")) return false;

    Expr* init = parse_expr();
    if (!init) return false;

    if (!expect(TokenKind::Semicolon, "Expected ';' after scalar let")) return false;

    out.kind = Stmt::Kind::LetScalar;
    out.letscalar.type = ty;
    out.letscalar.name = name;
    out.letscalar.init = init;
    return true;
}

bool Parser::parse_if(Stmt& out) {
    expect(TokenKind::KwIf, "Expected if");

    if (!expect(TokenKind::LParen, "Expected '(' after if")) return false;
    Expr* cond = parse_expr();
    if (!cond) return false;
    if (!expect(TokenKind::RParen, "Expected ')' after if condition")) return false;

    IfStmt is;
    is.cond = cond;

    if (!parse_block(*current_fn, is.then_body)) return false;

    if (match(TokenKind::KwElse)) {
        if (!parse_block(*current_fn, is.else_body)) return false;
    }

    out.kind = Stmt::Kind::If;
    out.ifs = is;
    return true;
}

bool Parser::parse_operand(Operand& out) {
    if (!expect(TokenKind::Identifier, "Expected operand base identifier")) return false;
    out.base = t[i-1].lexeme;

    if (!expect(TokenKind::LBracket, "Expected '[' in operand")) return false;
    if (!expect(TokenKind::Number, "Expected integer index in operand")) return false;

    out.index = static_cast<size_t>(std::strtoull(t[i-1].lexeme.c_str(), nullptr, 10));

    if (!expect(TokenKind::RBracket, "Expected ']' in operand")) return false;

  return true;
}

static bool is_param_gate(TokenKind tk) { // parameterized gate stuff
    return tk == TokenKind::KwRX || tk == TokenKind::KwRY || tk == TokenKind::KwRZ || tk == TokenKind::KwPHASE;
}

bool Parser::parse_gate_stmt(Stmt& out) {
    TokenKind tk = cur().kind;
    ++i;

    GateStmt gs;
    gs.has_param = false;
    gs.param = 0.0;

    GateStmtKind gk;
    if (tk == TokenKind::KwH) gk = GateStmtKind::H;
    else if (tk == TokenKind::KwX) gk = GateStmtKind::X;
    else if (tk == TokenKind::KwY) gk = GateStmtKind::Y;
    else if (tk == TokenKind::KwZ) gk = GateStmtKind::Z;
    else if (tk == TokenKind::KwS) gk = GateStmtKind::S;
    else if (tk == TokenKind::KwT) gk = GateStmtKind::T;
    else if (tk == TokenKind::KwRX) gk = GateStmtKind::RX;
    else if (tk == TokenKind::KwRY) gk = GateStmtKind::RY;
    else if (tk == TokenKind::KwRZ) gk = GateStmtKind::RZ;
    else if (tk == TokenKind::KwPHASE) gk = GateStmtKind::PHASE;
    else if (tk == TokenKind::KwCX) gk = GateStmtKind::CX;
    else if (tk == TokenKind::KwCZ) gk = GateStmtKind::CZ;
    else if (tk == TokenKind::KwCNOT) gk = GateStmtKind::CNOT;
    else if (tk == TokenKind::KwSWAP) gk = GateStmtKind::SWAP;
    else if (tk == TokenKind::KwMEASURE) gk = GateStmtKind::MEASURE;
    else gk = GateStmtKind::RESET;

    gs.kind = gk;

    // Parse parameter list for RX/RY/RZ/PHASE: GateName '(' Number ')'
    // Only integer numbers, stored as double, bc floats don't work yet
    if (is_param_gate(tk)) {
        if (!expect(TokenKind::LParen, "Expected '(' after parameterized gate")) return false;
        if (!expect(TokenKind::Number, "Expected numeric parameter for gate")) return false;
        gs.has_param = true;
        gs.param = std::strtod(t[i-1].lexeme.c_str(), nullptr);
        if (!expect(TokenKind::RParen, "Expected ')' after gate parameter")) return false;
    }

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

// expressions
Expr* Parser::parse_expr() { return parse_comparison(); }

Expr* Parser::parse_comparison() {
    Expr* left = parse_term();
    if (!left) return nullptr;

    while (true) {
        ExprKind k;
        if (match(TokenKind::EqEq)) k = ExprKind::Eq;
        else if (match(TokenKind::NotEq)) k = ExprKind::Ne;
        else if (match(TokenKind::Lt)) k = ExprKind::Lt;
        else if (match(TokenKind::LtEq)) k = ExprKind::Le;
        else if (match(TokenKind::Gt)) k = ExprKind::Gt;
        else if (match(TokenKind::GtEq)) k = ExprKind::Ge;
        else break;

        Expr* right = parse_term();
        if (!right) return nullptr;

        Expr* e = current_fn->arena.make();
        e->kind = k;
        e->left = left;
        e->right = right;
        left = e;
    }

    return left;
}

Expr* Parser::parse_term() {
    Expr* left = parse_factor();
    if (!left) return nullptr;

    while (true) {
        ExprKind k;
        if (match(TokenKind::Plus)) k = ExprKind::Add;
        else if (match(TokenKind::Minus)) k = ExprKind::Sub;
        else break;

        Expr* right = parse_factor();
        if (!right) return nullptr;

        Expr* e = current_fn->arena.make();
        e->kind = k;
        e->left = left;
        e->right = right;
        left = e;
    }

    return left;
}

Expr* Parser::parse_factor() {
    Expr* left = parse_primary();
    if (!left) return nullptr;

    while (true) {
        ExprKind k;
        if (match(TokenKind::Star)) k = ExprKind::Mul;
        else if (match(TokenKind::Slash)) k = ExprKind::Div;
        else break;

        Expr* right = parse_primary();
        if (!right) return nullptr;

        Expr* e = current_fn->arena.make();
        e->kind = k;
        e->left = left;
        e->right = right;
        left = e;
    }

    return left;
}

Expr* Parser::parse_primary() {
    if (match(TokenKind::Number)) {
        Expr* e = current_fn->arena.make();
        e->kind = ExprKind::IntLit;
        e->int_value = std::strtoll(t[i-1].lexeme.c_str(), nullptr, 10);
        return e;
    }

    if (match(TokenKind::KwTrue)) {
        Expr* e = current_fn->arena.make();
        e->kind = ExprKind::BoolLit;
        e->bool_value = true;
        return e;
    }

    if (match(TokenKind::KwFalse)) {
        Expr* e = current_fn->arena.make();
        e->kind = ExprKind::BoolLit;
        e->bool_value = false;
        return e;
    }

    if (match(TokenKind::Identifier)) {
        String name = t[i-1].lexeme;

        if (match(TokenKind::LParen)) {
            Expr* call = current_fn->arena.make();
            call->kind = ExprKind::Call;
            call->call_name = name;

            if (!at(TokenKind::RParen)) {
                while (true) {
                    Expr* a = parse_expr();
                    if (!a) return nullptr;
                    call->call_args.push_back(a);
                    if (match(TokenKind::Comma)) continue;
                    break;
                }
            }

            if (!expect(TokenKind::RParen, "Expected ')' after call args")) return nullptr;
            return call;
        }

        Expr* e = current_fn->arena.make();
        e->kind = ExprKind::Var;
        e->var_name = name;
        return e;
    }

    if (match(TokenKind::LParen)) {
        Expr* e = parse_expr();
        if (!expect(TokenKind::RParen, "Expected ')'")) return nullptr;
        return e;
    }

    error("Expected expression");
    return nullptr;
}

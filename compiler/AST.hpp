#ifndef QUELLC_AST_HPP
#define QUELLC_AST_HPP

#include "../datastructs/DynamicArray.hpp"
#include "../datastructs/String.hpp"
#include <cstddef>

enum class GateStmtKind { // types of gate statements
    H,X,Y,Z,S,T,
    CX,CZ,CNOT,
    SWAP,
    MEASURE,
    RESET
};

struct Operand { // only ident[number]
    String base;
    size_t index = 0;
};

struct LetRegDecl { // declare a register
    enum class Kind {QReg, BReg } kind;
    String name;
    size_t size = 0;
};

// New classical expression and statement nodes
enum class ScalarType {
    Int,
    Bool
};

enum class ExprKind {
    IntLit,
    BoolLit,
    Var,

    Call, // for function calls

    Add,
    Sub,
    Mul,
    Div,

    Eq,
    Ne,
    Lt,
    Le,
    Gt,
    Ge
};

struct Expr {
    ExprKind kind = ExprKind::IntLit;

    // payload
    long long int_value = 0;
    bool bool_value = false;
    String var_name;

    // for calls
    String call_name;
    DynamicArray<Expr*> call_args;

    // children (binary ops)
    Expr* left = nullptr;
    Expr* right = nullptr;
};

// Simple owning arena so we don't need std::vector or smart pointers bc no outside datastructs
struct ExprArena {
    DynamicArray<Expr*> nodes;

    Expr* make() {
        Expr* e = new Expr();
        nodes.push_back(e);
        return e;
    }

    ~ExprArena() {
        for (size_t i = 0; i < nodes.get_size(); ++i) delete nodes[i];
    }
};

struct LetScalarDecl {
    ScalarType type = ScalarType::Int;
    String name;
    Expr* init = nullptr; // required for v1
};

struct GateStmt { // gate statement applied on place
    GateStmtKind kind;
    DynamicArray<Operand> ops;
};

struct IfStmt {
    Expr* cond = nullptr;
    DynamicArray<struct Stmt> then_body;
    DynamicArray<struct Stmt> else_body;
};

// General type references for function params/returns, pass "anything" via the call stack by encoding as ints/pairs
enum class TypeKind {
    Void,
    Int,
    Bool,
    QReg,
    BReg
};

struct TypeRef {
    TypeKind kind = TypeKind::Void;
    size_t reg_size = 0; // only used for qreg<bignum> and breg<bignum>
};

struct Param {
    String name;
    TypeRef type;
};

struct Stmt { // statement
    enum class Kind { LetReg, LetScalar, If, Gate, Return, Empty } kind;

    LetRegDecl letreg;
    LetScalarDecl letscalar;
    IfStmt ifs;

    GateStmt gate;

    // Return statement
    Expr* ret_expr = nullptr; // null means "return;" (only valid for void)
};

struct Function { // function - main thing bc functional langugae
    String name;

    // Parameters + return type
    DynamicArray<Param> params;
    TypeRef return_type;

    ExprArena arena;           // owns expression nodes used in this function
    DynamicArray<Stmt> body;
};

struct Program { // program is just an array of functions
    DynamicArray<Function> functions;
};

#endif

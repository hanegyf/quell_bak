#ifndef QUELLC_AST_HPP
#define QUELLC_AST_HPP

#include "../datastructs/DynamicArray.hpp"
#include "../datastructs/String.hpp"
#include <cstddef>

enum class GateStmtKind { // types of gate statements
    H,X,Y,Z,S,T,
    RX, RY, RZ, PHASE, // parameterized gates
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

// classical expression and statement nodes
enum class ScalarType { Int, Bool };

enum class ExprKind {
    IntLit,
    BoolLit,
    Var,

    Call,

    Add, Sub, Mul, Div,
    Eq, Ne, Lt, Le, Gt, Ge
};

struct Expr {
    ExprKind kind = ExprKind::IntLit;

    long long int_value = 0;
    bool bool_value = false;
    String var_name;

    String call_name;
    DynamicArray<Expr*> call_args;

    Expr* left = nullptr;
    Expr* right = nullptr;
};

struct ExprArena {
    DynamicArray<Expr*> nodes;

    Expr* make() {
        Expr* e = new Expr();
        nodes.push_back(e);
        return e;
    }

    // Move-only: owns the Expr* pointers
    ExprArena() = default;
    ExprArena(ExprArena&&) = default;
    ExprArena& operator=(ExprArena&&) = default;
    ExprArena(const ExprArena&) = delete;
    ExprArena& operator=(const ExprArena&) = delete;

    ~ExprArena() {
        for (size_t i = 0; i < nodes.get_size(); ++i) delete nodes[i];
    }
};

struct LetScalarDecl {
    ScalarType type = ScalarType::Int;
    String name;
    Expr* init = nullptr;
};

struct GateStmt {
    GateStmtKind kind;
    DynamicArray<Operand> ops;

    // optional parameter for RX/RY/RZ/PHASE
    bool has_param = false;
    double param = 0.0;
};

struct IfStmt {
    Expr* cond = nullptr;
    DynamicArray<struct Stmt> then_body;
    DynamicArray<struct Stmt> else_body;
};

enum class TypeKind { Void, Int, Bool, QReg, BReg };

struct TypeRef {
    TypeKind kind = TypeKind::Void;
    size_t reg_size = 0;
};

struct Param {
    String name;
    TypeRef type;
};

struct Stmt {
    enum class Kind { LetReg, LetScalar, If, Gate, Return, Empty } kind;

    LetRegDecl letreg;
    LetScalarDecl letscalar;
    IfStmt ifs;

    GateStmt gate;

    Expr* ret_expr = nullptr;
};

struct Function {
    String name;

    DynamicArray<Param> params;
    TypeRef return_type;

    ExprArena arena;
    DynamicArray<Stmt> body;

    // Move-only because ExprArena is move-only
    Function() = default;
    Function(Function&&) = default;
    Function& operator=(Function&&) = default;
    Function(const Function&) = delete;
    Function& operator=(const Function&) = delete;
};

struct Program {
    DynamicArray<Function> functions;
};

#endif

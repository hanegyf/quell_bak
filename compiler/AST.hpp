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

struct GateStmt { // gate statement applied on place
    GateStmtKind kind;
    DynamicArray<Operand> ops;
};

struct Stmt { // normla statement (3 types - let, gate, or nothing)
    enum class Kind { LetReg, Gate, Empty } kind;
    LetRegDecl letreg;
    GateStmt gate;
};

struct Function { // function - main thing bc functional langugae
    String name;
    DynamicArray<Stmt> body;
};

struct Program { // program is just an array of functions
    DynamicArray<Function> functions;
};

#endif

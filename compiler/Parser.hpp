#ifndef QUELLC_PARSER_HPP
#define QUELLC_PARSER_HPP

#include "../datastructs/DynamicArray.hpp"
#include "../datastructs/String.hpp"
#include "AST.hpp"
#include "Token.hpp"

struct ParseResult { // describes if parsed correctly, all parsing errors, and result
    bool ok = true;
    DynamicArray<String> errors;
    Program program;
};

class Parser { // actual parser
    public:
        explicit Parser(const DynamicArray<Token>& toks); // toks is fun to say
        ParseResult parse_program(); // do everything

    private:
        const DynamicArray<Token>& t; // toks is too much work to write
        size_t i; // current place
        DynamicArray<String>* errors; // error handling
        bool ok;

        const Token& cur() const; // current token
        bool at(TokenKind k) const;
        bool match(TokenKind k);
        bool expect(TokenKind k, const char* msg);

        void error(const char* msg);

        // parse all different token stuff
        bool parse_function(Function& out);
        bool parse_block(Function& fn);
        bool parse_stmt(Stmt& out);

        bool parse_let_reg(Stmt& out);
        bool parse_gate_stmt(Stmt& out);
        bool parse_operand(Operand& out);

        bool parse_reg_type_size(size_t& outSize, bool& isQReg);
};

#endif

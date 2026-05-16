#ifndef QUELLC_LOWER_HPP
#define QUELLC_LOWER_HPP

#include "../datastructs/DynamicArray.hpp"
#include "../datastructs/HashTable.hpp"
#include "../datastructs/String.hpp"
#include "AST.hpp"

struct LowerResult {
    bool ok = true;
    DynamicArray<String> errors;

    size_t num_qubits = 0;
    size_t num_bits = 0;

    // backend .q instruction lines only, no header lines
    DynamicArray<String> instr_lines;
};

LowerResult lower_to_q(const Program& p); // lowers a prsed program to main.q

#endif

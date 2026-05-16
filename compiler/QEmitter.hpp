#ifndef QUELLC_QEMITTER_HPP
#define QUELLC_QEMITTER_HPP

#include "../datastructs/DynamicArray.hpp"
#include "../datastructs/String.hpp"
#include <cstddef>

struct QEmitOptions {
    bool emit_shots = false;
    size_t shots = 1;
    bool emit_end_measure = false; // bare MEASURE line
};

class QEmitter { // class to emit into a .q format
    public:
        static DynamicArray<String> emit(size_t qubits, size_t bits, const DynamicArray<String>& instr_lines, const QEmitOptions& opt);
        static String make_header_line(const char* key, size_t value);
        static String make_gate1(const char* name, size_t a);
        static String make_gate2(const char* name, size_t a, size_t b);
};

#endif

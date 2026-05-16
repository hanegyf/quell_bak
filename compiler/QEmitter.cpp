#include "QEmitter.hpp"
#include <cstdio>

String QEmitter::make_header_line(const char* key, size_t value) { // format into a header line
    char buf[128];
    std::snprintf(buf, sizeof(buf), "%s %zu", key, value);
    return String(buf);
}

String QEmitter::make_gate1(const char* name, size_t a) { // format a gate (basically same as header line, just different for clarity and also because i started typing and am too lazy to delete)
    char buf[128];
    std::snprintf(buf, sizeof(buf), "%s %zu", name, a);
    return String(buf);
}

String QEmitter::make_gate2(const char* name, size_t a, size_t b) { // format other gate with mroe than one param
    char buf[128];
    std::snprintf(buf, sizeof(buf), "%s %zu %zu", name, a, b);
    return String(buf);
}

DynamicArray<String> QEmitter::emit(size_t qubits, size_t bits, const DynamicArray<String>& instr_lines, const QEmitOptions& opt) { // emit into .Q format
    DynamicArray<String> out;
    out.push_back(make_header_line("QUBITS", qubits)); // header
    out.push_back(make_header_line("BITS", bits)); // header
    if (opt.emit_shots) out.push_back(make_header_line("SHOTS", opt.shots)); // shots if wanted
    out.push_back(String("")); // empty line

    for (size_t i = 0; i < instr_lines.get_size(); ++i) // instructions
        out.push_back(instr_lines[i]);

    if (opt.emit_end_measure) // measure if watned
        out.push_back(String("MEASURE"));

    return out;
}

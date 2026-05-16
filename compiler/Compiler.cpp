#include "Compiler.hpp"
#include "Lexer.hpp"
#include "Parser.hpp"
#include "Lower.hpp"
#include "QEmitter.hpp"
#include <fstream>

Compiler::Compiler() {}

bool Compiler::read_entire_file(const String& filepath, String& out_src, String& err) {
    std::ifstream file(filepath.c_str(), std::ios::binary);
    if (!file) {
        err = String("Failed to open source file");
        return false;
    }

    DynamicArray<char> buf;
    char ch;
    bool got = false;
    while (file.get(ch)) {
        buf.push_back(ch);
        got = true;
    }
    (void)got;
    buf.push_back('\0');
    out_src = String(&buf[0]);
    return true;
}

CompileResult Compiler::compile_file_to_q_lines(const String& source_filepath, const CompileOptions& opts) {
    CompileResult res;

    String src;
    String err;
    if (!read_entire_file(source_filepath, src, err)) {
        res.ok = false;
        res.errors.push_back(err);
        return res;
    }

    Lexer lex(src);
    LexResult lx = lex.lex();
    if (!lx.ok) {
        res.ok = false;
        for (size_t i = 0; i < lx.errors.get_size(); ++i)
            res.errors.push_back(lx.errors[i]);
        return res;
    }

    Parser parser(lx.tokens);
    ParseResult pr = parser.parse_program();
    if (!pr.ok) {
        res.ok = false;
        for (size_t i = 0; i < pr.errors.get_size(); ++i)
            res.errors.push_back(pr.errors[i]);
        return res;
    }

    LowerResult lr = lower_to_q(pr.program);
    if (!lr.ok) {
        res.ok = false;
        for (size_t i = 0; i < lr.errors.get_size(); ++i)
            res.errors.push_back(lr.errors[i]);
        return res;
    }

    QEmitOptions qopt;
    qopt.emit_shots = opts.emit_shots_header;
    qopt.shots = opts.default_shots;
    qopt.emit_end_measure = opts.emit_end_measure;

    res.q_lines = QEmitter::emit(lr.num_qubits, lr.num_bits, lr.instr_lines, qopt);
    return res;
}

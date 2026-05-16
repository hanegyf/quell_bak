#ifndef QUELLC_COMPILER_HPP
#define QUELLC_COMPILER_HPP

#include "../datastructs/DynamicArray.hpp"
#include "../datastructs/String.hpp"

struct CompileOptions {
    bool emit_shots_header = false;
    size_t default_shots = 1;
    bool emit_end_measure = false; // bare MEASURE at end
};

struct CompileResult {
    bool ok = true;
    DynamicArray<String> errors;
    DynamicArray<String> q_lines;
};

class Compiler {
    public:
        Compiler();
        CompileResult compile_file_to_q_lines(const String& source_filepath, const CompileOptions& opts);

    private:
        static bool read_entire_file(const String& filepath, String& out_src, String& err);
};
#endif

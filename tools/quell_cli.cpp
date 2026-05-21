#include "../backend/Backend.hpp"
#include "../compiler/Compiler.hpp"
#include "../datastructs/RNG.hpp"
#include "../datastructs/String.hpp"
#include "../datastructs/StateVector.hpp"
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <cstring>

static void usage() { // how to use it
  std::cerr
      << "Usage:\n"
      << "  quell <input.ql> [--shots N] [--seed N]\n"
      << "       [--emit-q out.q] [--hist out.txt] [--state out.txt]\n";
}

static bool streq(const char* a, const char* b) { // if strings are equal
  return std::strcmp(a, b) == 0;
}

static bool write_lines_to_file(const char* path, const DynamicArray<String>& lines) { // write the array of lines to a certain path
  std::ofstream out(path);
  if (!out) return false;
  for (size_t i = 0; i < lines.get_size(); ++i) {
    out << lines[i].c_str() << "\n";
  }
  return true;
}

static std::string make_temp_q_path(const std::string& input_path) {
  // Very simple temp naming: <input>.generated.q bc too lazy to use actual temp stuff
  return input_path + ".generated.q";
}

static void print_histogram_stdout(const HashTable<size_t, size_t>& hist) { // print out values from a histogram
  std::cout << "Histogram (outcome -> count):\n";
  hist.for_each([&](const size_t& key, const size_t& value) {
    std::cout << key << " -> " << value << "\n";
  });
}

static bool write_histogram_file(const char* path, const HashTable<size_t, size_t>& hist) { // save the histogram
  std::ofstream out(path);
  if (!out) return false;
  hist.for_each([&](const size_t& key, const size_t& value) {
    out << key << " " << value << "\n";
  });
  return true;
}

static void print_statevector_stdout(const Statevector& sv) { // print out statevector
  std::cout << "Statevector amplitudes:\n";
  for (size_t i = 0; i < sv.size(); ++i) {
    // Complex has get_real/get_imag in your tests usage
    std::cout << i << ": (" << sv[i].get_real() << ", " << sv[i].get_imag() << ")\n";
  }
}

static bool write_statevector_file(const char* path, const Statevector& sv) { // save the statevector to a file
  std::ofstream out(path);
  if (!out) return false;
  for (size_t i = 0; i < sv.size(); ++i) {
    out << i << " " << sv[i].get_real() << " " << sv[i].get_imag() << "\n";
  }
  return true;
}

int main(int argc, char** argv) { // main program
  if (argc < 2) {
    usage();
    return 1;
  }

  std::string input_ql;
  size_t shots = 1;
  size_t seed = 1234;

  const char* emit_q_path = nullptr;
  const char* hist_path = nullptr;
  const char* state_path = nullptr;

  input_ql = argv[1];

  for (int i = 2; i < argc; ++i) {
    if (streq(argv[i], "--shots")) {
      if (i + 1 >= argc) { std::cerr << "--shots requires a value\n"; return 1; }
      shots = static_cast<size_t>(std::strtoull(argv[++i], nullptr, 10));
      continue;
    }
    if (streq(argv[i], "--seed")) {
      if (i + 1 >= argc) { std::cerr << "--seed requires a value\n"; return 1; }
      seed = static_cast<size_t>(std::strtoull(argv[++i], nullptr, 10));
      continue;
    }
    if (streq(argv[i], "--emit-q")) {
      if (i + 1 >= argc) { std::cerr << "--emit-q requires a path\n"; return 1; }
      emit_q_path = argv[++i];
      continue;
    }
    if (streq(argv[i], "--hist")) {
      if (i + 1 >= argc) { std::cerr << "--hist requires a path\n"; return 1; }
      hist_path = argv[++i];
      continue;
    }
    if (streq(argv[i], "--state")) {
      if (i + 1 >= argc) { std::cerr << "--state requires a path\n"; return 1; }
      state_path = argv[++i];
      continue;
    }

    std::cerr << "Unknown argument: " << argv[i] << "\n";
    usage();
    return 1;
  }

  // 1. compile .ql -> .q lines
  Compiler comp;
  CompileOptions copt;
  copt.emit_shots_header = true; // CLI controls shots
  copt.default_shots = shots;
  copt.emit_end_measure = false; // only if needed later

  CompileResult cr = comp.compile_file_to_q_lines(String(input_ql.c_str()), copt);
  if (!cr.ok) {
    std::cerr << "Compilation failed:\n";
    for (size_t i = 0; i < cr.errors.get_size(); ++i) {
      std::cerr << "  - " << cr.errors[i].c_str() << "\n";
    }
    return 1;
  }

  // 2. write generated .q to disk
  std::string temp_q = make_temp_q_path(input_ql);
  if (!write_lines_to_file(temp_q.c_str(), cr.q_lines)) {
    std::cerr << "Failed to write generated .q file: " << temp_q << "\n";
    return 1;
  }

  if (emit_q_path) {
    if (!write_lines_to_file(emit_q_path, cr.q_lines)) {
      std::cerr << "Failed to write --emit-q file\n";
      return 1;
    }
  }

  // 3. run backend
  Backend backend;
  backend.load(String(temp_q.c_str()));

  RNG rng(seed);
  backend.run(rng);

  // 4. output results
  const auto& hist = backend.get_histogram();
  print_histogram_stdout(hist);
  if (hist_path) {
    if (!write_histogram_file(hist_path, hist)) {
      std::cerr << "Failed to write histogram file\n";
      return 1;
    }
  }

  // statevector is last state from final shot
  const Statevector& sv = backend.get_statevector();
  print_statevector_stdout(sv);
  if (state_path) {
    if (!write_statevector_file(state_path, sv)) {
      std::cerr << "Failed to write statevector file\n";
      return 1;
    }
  }

  // 5. delete temp file
  std::remove(temp_q.c_str());

  return 0;
}

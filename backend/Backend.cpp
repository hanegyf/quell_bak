#include "Backend.hpp"
#include "../datastructs/String.hpp"
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <stdexcept>

static bool
is_comment_or_empty(const String &line) { // check if an instruction is just an
                                          // empty line or has a comment
  const char *s = line.c_str();
  for (size_t i = 0; s[i] != '\0'; ++i) { // while not null terminator
    if (s[i] == '#')
      return true; // # == comment
    if (!std::isspace(static_cast<unsigned char>(s[i])))
      return false; // anything else
  }
  return true; // empty
}

static void skip_ws(const char *&p) { // skip whitespace, dereferences p
  while (*p && std::isspace(static_cast<unsigned char>(*p))) {
    ++p;
  }
}

static String read_token(const char *&p) { // read value of a token
  skip_ws(p);                              // go until not whitespace
  DynamicArray<char> buffer;
  while (*p && !std::isspace(
                   static_cast<unsigned char>(*p))) { // add values to buffer
    buffer.push_back(*p);
    ++p;
  }
  buffer.push_back('\0');    // push a null terminator
  return String(&buffer[0]); // return reference of first element
}

static bool
has_more_tokens(const char *p) { // if a line has more tokens remaining
  skip_ws(p);
  return *p !=
         '\0'; // get if line is only whitespace and then the null terminator
}

static size_t parse_size_t(const char *&p) { // convert token to size_t
  skip_ws(p);
  char *end = nullptr;
  unsigned long long value = std::strtoull(
      p, &end, 10); // convert string into large unsigned integer in base 10
  if (end == p)     // if at end
    throw std::runtime_error("Expected integer");
  p = end;
  return static_cast<size_t>(value);
}

static long long parse_i64(const char *&p) { // convert token to signed int64
  skip_ws(p);
  char *end = nullptr;
  long long value = std::strtoll(p, &end, 10);
  if (end == p)
    throw std::runtime_error("Expected integer");
  p = end;
  return value;
}

static double parse_double(const char *&p) { // convert token to double
  skip_ws(p);
  char *end = nullptr;
  double value = std::strtod(p, &end); // convert string into double
  if (end == p)
    throw std::runtime_error("Expected double");
  p = end; // set token to null pointer
  return value;
}

static bool read_line(std::ifstream &file,
                      String &out) { // file to read and line to return
  DynamicArray<char> buffer;         // buffer DynamicArray
  char ch;
  bool got = false;      // if anything was read
  while (file.get(ch)) { // while there is something to get
    if (ch == '\r')      // carriage return (overwrite back to beginning / does
                         // nothing but not saved)
      continue;
    if (ch == '\n') // new line (end of line)
      break;
    buffer.push_back(ch);
    got = true; // if a character had been saved
  }
  if (!got && !file) // if bad file, return false (out not changed)
    return false;
  buffer.push_back('\0');   // push back null terminator - end of string
  out = String(&buffer[0]); // set out to new line
  return true;              // out was changed
}

Backend::Backend()
    : num_qubits(0), num_bits(0), num_shots(1), last_state(0),
      has_last_state(false) {} // default constructor

GateID Backend::parse_gate_name(
    const String &name) const { // parse string name to GateID
  if (name == "I")
    return GateID::I;
  if (name == "H")
    return GateID::H;
  if (name == "X")
    return GateID::X;
  if (name == "Y")
    return GateID::Y;
  if (name == "Z")
    return GateID::Z;
  if (name == "S")
    return GateID::S;
  if (name == "T")
    return GateID::T;
  if (name == "S_DAG")
    return GateID::S_DAG;
  if (name == "T_DAG")
    return GateID::T_DAG;
  if (name == "RX")
    return GateID::RX;
  if (name == "RY")
    return GateID::RY;
  if (name == "RZ")
    return GateID::RZ;
  if (name == "PHASE")
    return GateID::PHASE;
  if (name == "CNOT")
    return GateID::CNOT;
  if (name == "CZ")
    return GateID::CZ;
  throw std::runtime_error(
      "Unkown gate name"); // if none of accepted ones, throw exception
}

Matrix Backend::resolve_gate(
    GateID id,
    const std::optional<double> &param) const { // convert GateID to matrix
  switch (id) {   // massive switch case depending on gate
  case GateID::I: // identity
    return Matrix::identity();
  case GateID::H: // hadamard
    return Matrix(Complex(1, 0), Complex(1, 0), Complex(1, 0), Complex(-1, 0)) *
           Complex(1.0 / std::sqrt(2.0), 0);
  case GateID::X: // pauli x
    return Matrix(Complex(0, 0), Complex(1, 0), Complex(1, 0), Complex(0, 0));
  case GateID::Y: // pauli y
    return Matrix(Complex(0, 0), Complex(0, -1), Complex(0, 1), Complex(0, 0));
  case GateID::Z: // pauli z
    return Matrix(Complex(1, 0), Complex(0, 0), Complex(0, 0), Complex(-1, 0));
  case GateID::S: // S gate (complex phase)
    return Matrix(Complex(1, 0), Complex(0, 0), Complex(0, 0), Complex(0, 1));
  case GateID::T: // T gate (rotate by PI/4)
    return Matrix(Complex(1, 0), Complex(0, 0), Complex(0, 0),
                  Complex(std::cos(M_PI / 4.0), std::sin(M_PI / 4.0)));
  case GateID::S_DAG: // S dagger (negative i)
    return Matrix(Complex(1, 0), Complex(0, 0), Complex(0, 0), Complex(0, -1));
  case GateID::RX: { // rotate around X axis, parametrized
    if (!param)
      throw std::runtime_error("RX requires parameter");
    double p = *param;
    return Matrix(Complex(std::cos(p / 2), 0), Complex(-std::sin(p / 2), 0),
                  Complex(0, -std::sin(p / 2)), Complex(std::cos(p / 2), 0));
  }
  case GateID::RY: { // Rotate around Y axis, parameterized
    if (!param)
      throw std::runtime_error("RY requires parameter");
    double p = *param;
    return Matrix(Complex(std::cos(p / 2), 0), Complex(-std::sin(p / 2), 0),
                  Complex(std::sin(p / 2), 0), Complex(std::cos(p / 2), 0));
  }
  case GateID::RZ: { // Rotate around Z axis, parameterized
    if (!param)
      throw std::runtime_error("RZ requires parameter");
    double p = *param;
    return Matrix(Complex(std::cos(-p / 2), std::sin(-p / 2)), Complex(0, 0),
                  Complex(0, 0), Complex(std::cos(p / 2), std::sin(p / 2)));
  }
  case GateID::PHASE: { // apply phase to state, parameterized
    if (!param)
      throw std::runtime_error("PHASE requires parameter");
    double p = *param;
    return Matrix(Complex(1, 0), Complex(0, 0), Complex(0, 0),
                  Complex(std::cos(p), std::sin(p)));
  }
  case GateID::CNOT: // CNOT matrix is just pauli x
    return resolve_gate(GateID::X, std::nullopt);
  case GateID::CZ: // CZ matrix is pauli Z
    return resolve_gate(GateID::Z, std::nullopt);
  }
  return Matrix::identity(); // if unknown somehow, just use identity
}

Instruction Backend::parse_instruction_line(
    const String &line) const { // pasre a single line
  const char *p = line.c_str();
  String opcode = read_token(p);

  if (opcode == String("INIT")) { // initialize certain qubit back to |0>
    Instruction instr;
    instr.op = OpCode::Init;
    instr.target = parse_size_t(p); // get qubit index (size_t)
    return instr;
  }

  // TODO: make MEASURE alone not end code, but end code when no more tokens
  if (opcode == String("MEASURE")) { // measuring one qubit or all
    if (!has_more_tokens(p)) {       // measure everything
      Instruction instr;
      instr.op = OpCode::End; // end code when all measured
      return instr;
    }
    Instruction instr;
    instr.op = OpCode::Measure;     // Measure OpCode
    instr.target = parse_size_t(p); // get qubit index
    instr.bit = parse_size_t(p);    // get bit index (also classical register index)
    return instr;
  }

  // New classical VM ops
  // Example layout bc its confusing
  //   MOVI dst imm
  //   MOV  dst src
  //   ADD  dst a b
  //   CMPEQ dst a b
  //   JMP offset
  //   JZ  reg offset
  //   JNZ reg offset
  if (opcode == String("MOVI")) {
    Instruction instr;
    instr.op = OpCode::MovImm;
    instr.dst = parse_size_t(p);
    instr.imm = parse_i64(p);
    return instr;
  }
  if (opcode == String("MOV")) {
    Instruction instr;
    instr.op = OpCode::Mov;
    instr.dst = parse_size_t(p);
    instr.src1 = parse_size_t(p);
    return instr;
  }
  if (opcode == String("ADD") || opcode == String("SUB") ||
      opcode == String("MUL") || opcode == String("DIV")) {
    Instruction instr;
    instr.op = (opcode == String("ADD")) ? OpCode::Add :
               (opcode == String("SUB")) ? OpCode::Sub :
               (opcode == String("MUL")) ? OpCode::Mul : OpCode::Div;
    instr.dst = parse_size_t(p);
    instr.src1 = parse_size_t(p);
    instr.src2 = parse_size_t(p);
    return instr;
  }
  if (opcode == String("CMPEQ") || opcode == String("CMPNE") ||
      opcode == String("CMPLT") || opcode == String("CMPLE") ||
      opcode == String("CMPGT") || opcode == String("CMPGE")) {
    Instruction instr;
    instr.op = (opcode == String("CMPEQ")) ? OpCode::CmpEq :
               (opcode == String("CMPNE")) ? OpCode::CmpNe :
               (opcode == String("CMPLT")) ? OpCode::CmpLt :
               (opcode == String("CMPLE")) ? OpCode::CmpLe :
               (opcode == String("CMPGT")) ? OpCode::CmpGt : OpCode::CmpGe;
    instr.dst = parse_size_t(p);
    instr.src1 = parse_size_t(p);
    instr.src2 = parse_size_t(p);
    return instr;
  }
  if (opcode == String("JMP")) {
    Instruction instr;
    instr.op = OpCode::Jmp;
    instr.imm = parse_i64(p); // pc-relative offset
    return instr;
  }
  if (opcode == String("JZ")) {
    Instruction instr;
    instr.op = OpCode::JmpIfZero;
    instr.src1 = parse_size_t(p); // reg
    instr.imm = parse_i64(p);     // offset
    return instr;
  }
  if (opcode == String("JNZ")) {
    Instruction instr;
    instr.op = OpCode::JmpIfNonZero;
    instr.src1 = parse_size_t(p); // reg
    instr.imm = parse_i64(p);     // offset
    return instr;
  }

  // Function/call stack ops ----
  //   PUSH reg
  //   POP reg
  //   ARG dst index
  //   CALL target_pc nargs
  //   RET nargs
  if (opcode == String("PUSH")) {
    Instruction instr;
    instr.op = OpCode::Push;
    instr.src1 = parse_size_t(p);
    return instr;
  }
  if (opcode == String("POP")) {
    Instruction instr;
    instr.op = OpCode::Pop;
    instr.dst = parse_size_t(p);
    return instr;
  }
  if (opcode == String("ARG")) {
    Instruction instr;
    instr.op = OpCode::ArgLoad;
    instr.dst = parse_size_t(p);
    instr.imm = parse_i64(p); // arg index
    return instr;
  }
  if (opcode == String("CALL")) {
    Instruction instr;
    instr.op = OpCode::Call;
    instr.imm = parse_i64(p);  // target pc
    instr.imm2 = parse_i64(p); // nargs
    return instr;
  }
  if (opcode == String("RET")) {
    Instruction instr;
    instr.op = OpCode::Ret;
    instr.imm = parse_i64(p); // nargs
    return instr;
  }

  // None of the above == gate
  Instruction instr;
  instr.op = OpCode::Gate;
  instr.gate = parse_gate_name(opcode);

  if (instr.gate == GateID::CNOT ||
      instr.gate == GateID::CZ) {    // if has a control
    instr.control = parse_size_t(p); // control first, then target
    instr.target = parse_size_t(p);
    return instr;
  }

  if (instr.gate == GateID::RX || instr.gate == GateID::RY ||
      instr.gate == GateID::RZ || instr.gate == GateID::PHASE) {
    instr.parameter = parse_double(p); // parameter first
    instr.target = parse_size_t(p);    // then target
    return instr;
  }

  // if non parameterized non controlled
  instr.target = parse_size_t(p);
  return instr;
}

void Backend::load(const String &filepath) { // load from file path to program
  std::ifstream file(filepath.c_str());
  if (!file)
    throw std::runtime_error("Failed to open file");

  program =
      DynamicArray<Instruction>(); // program is a dynamic array of instructions
  histogram = HashTable<size_t, size_t>(); // histogram to be returned
  num_qubits = 0;
  num_bits = 0;
  num_shots = 1;
  has_last_state = 0;

  bool header_locked = false; // if still in the header
  String line;

  while (read_line(file, line)) { // while not at the end of the code
    if (is_comment_or_empty(line))
      continue;

    const char *p = line.c_str(); // pointer to line
    String token = read_token(p); // convert to tokens

    if (!header_locked &&
        (token == String("QUBITS") || token == String("BITS") ||
         token == String("SHOTS"))) { // if in the header still
      if (token == String("QUBITS")) {
        num_qubits = parse_size_t(p); // num qubits
      } else if (token == String("BITS")) {
        num_bits = parse_size_t(p); // num bits / classical reg count
      } else if (token == String("SHOTS")) {
        num_shots = parse_size_t(p); // num shots (default to 1)
      }
      continue; // continue being in the header
    }

    // no longer in header

    header_locked = true;
    Instruction instr = parse_instruction_line(line); // instruction from line
    program.push_back(instr);
    if (instr.op == OpCode::End)
      break; // if end of code (measurement of all for now)
  }

  if (num_qubits == 0)
    throw std::runtime_error("QUBITS must be set"); // missing in header
  if (num_bits == 0)
    throw std::runtime_error("BITS must be set"); // missing in header
}

void Backend::collapse_qubit(Statevector &state,
                             size_t qubit, // collapse a qubit's superposition
                             size_t value) const {
  size_t dim = state.size();
  size_t mask = 1ULL << qubit;

  double norm = 0.0;
  for (size_t i = 0; i < dim; ++i) {
    if (((i & mask) != 0) != (value != 0))
      state[i] = Complex(0.0, 0.0);
    else
      norm += state[i].norm();
  }

  if (norm <= 0.0)
    throw std::runtime_error("Collapse resulted in zero norm");

  double inv = 1.0 / std::sqrt(norm);
  for (size_t i = 0; i < dim; ++i) {
    state[i] *= inv;
  }
}

void Backend::reset_qubit(Statevector &state, size_t qubit) const {
  size_t dim = state.size();
  size_t mask = 1ULL << qubit;

  for (size_t i = 0; i < dim; ++i) {
    if ((i & mask) == 0) {
      size_t j = i | mask;
      state[i] = state[i] + state[j];
      state[j] = Complex(0.0, 0.0);
    }
  }

  state.normalize();
}

void Backend::measure_qubit(Statevector &state, size_t qubit,
                            size_t &result, // measure a single qubit
                            RNG &rng) const {
  size_t dim = state.size();
  size_t mask = 1ULL << qubit;
  double p0 = 0.0;

  for (size_t i = 0; i < dim; ++i) {
    if ((i & mask) == 0)
      p0 += state[i].norm();
  }

  // Numerical safety (important once you start doing lots of classical control)
  if (p0 < 0.0) p0 = 0.0;
  if (p0 > 1.0) p0 = 1.0;

  static const double EPS = 1e-15;
  if (p0 <= EPS) {
    result = 1;
  } else if (p0 >= 1.0 - EPS) {
    result = 0;
  } else {
    double r = rng.uniform();
    result = (r < p0) ? 0 : 1;
  }

  collapse_qubit(state, qubit, result);
}

size_t Backend::encode_bits(
    const DynamicArray<size_t> &bits) const { // encode bit into bitstring
  size_t value = 0;
  for (size_t i = 0; i < bits.get_size(); ++i) {
    if (bits[i])
      value |= (1ULL << i);
  }
  return value;
}

void Backend::run(RNG &rng) {              // run code
  histogram = HashTable<size_t, size_t>(); // results
  has_last_state = false;

  for (size_t shot = 0; shot < num_shots; ++shot) {
    Statevector state(num_qubits);

    DynamicArray<size_t> classical_bits;
    for (size_t i = 0; i < num_bits; ++i)
      classical_bits.push_back(0);

    DynamicArray<long long> creg;
    for (size_t i = 0; i < num_bits; ++i)
      creg.push_back(0);

    DynamicArray<size_t> locked;
    for (size_t i = 0; i < num_qubits; ++i)
      locked.push_back(0);

    // Call stack
    Stack<long long> value_stack;    // values + args + return values
    Stack<long long> return_pc_stack;
    Stack<long long> frame_base_stack;

    long long pc = 0;
    while (pc >= 0 && static_cast<size_t>(pc) < program.get_size()) {
      const Instruction &instr = program[static_cast<size_t>(pc)];

      // classical execution
      if (instr.op == OpCode::MovImm) {
        if (instr.dst >= num_bits) throw std::runtime_error("MOVI dst out of range");
        creg[instr.dst] = instr.imm;
        pc += 1;
        continue;
      }
      if (instr.op == OpCode::Mov) {
        if (instr.dst >= num_bits || instr.src1 >= num_bits) throw std::runtime_error("MOV reg out of range");
        creg[instr.dst] = creg[instr.src1];
        pc += 1;
        continue;
      }
      if (instr.op == OpCode::Add || instr.op == OpCode::Sub ||
          instr.op == OpCode::Mul || instr.op == OpCode::Div) {
        if (instr.dst >= num_bits || instr.src1 >= num_bits || instr.src2 >= num_bits)
          throw std::runtime_error("ALU reg out of range");
        long long a = creg[instr.src1];
        long long b = creg[instr.src2];
        if (instr.op == OpCode::Add) creg[instr.dst] = a + b;
        else if (instr.op == OpCode::Sub) creg[instr.dst] = a - b;
        else if (instr.op == OpCode::Mul) creg[instr.dst] = a * b;
        else {
          if (b == 0) throw std::runtime_error("DIV by zero");
          creg[instr.dst] = a / b;
        }
        pc += 1;
        continue;
      }
      if (instr.op == OpCode::CmpEq || instr.op == OpCode::CmpNe ||
          instr.op == OpCode::CmpLt || instr.op == OpCode::CmpLe ||
          instr.op == OpCode::CmpGt || instr.op == OpCode::CmpGe) {
        if (instr.dst >= num_bits || instr.src1 >= num_bits || instr.src2 >= num_bits)
          throw std::runtime_error("CMP reg out of range");
        long long a = creg[instr.src1];
        long long b = creg[instr.src2];
        bool v = false;
        if (instr.op == OpCode::CmpEq) v = (a == b);
        else if (instr.op == OpCode::CmpNe) v = (a != b);
        else if (instr.op == OpCode::CmpLt) v = (a < b);
        else if (instr.op == OpCode::CmpLe) v = (a <= b);
        else if (instr.op == OpCode::CmpGt) v = (a > b);
        else v = (a >= b);
        creg[instr.dst] = v ? 1 : 0;
        pc += 1;
        continue;
      }
      if (instr.op == OpCode::Jmp) {
        pc += instr.imm;
        continue;
      }
      if (instr.op == OpCode::JmpIfZero) {
        if (instr.src1 >= num_bits) throw std::runtime_error("JZ reg out of range");
        if (creg[instr.src1] == 0) pc += instr.imm;
        else pc += 1;
        continue;
      }
      if (instr.op == OpCode::JmpIfNonZero) {
        if (instr.src1 >= num_bits) throw std::runtime_error("JNZ reg out of range");
        if (creg[instr.src1] != 0) pc += instr.imm;
        else pc += 1;
        continue;
      }

      // Stack ops
      if (instr.op == OpCode::Push) {
        if (instr.src1 >= num_bits) throw std::runtime_error("PUSH reg out of range");
        value_stack.push(creg[instr.src1]);
        pc += 1;
        continue;
      }
      if (instr.op == OpCode::Pop) {
        if (instr.dst >= num_bits) throw std::runtime_error("POP reg out of range");
        if (value_stack.empty()) throw std::runtime_error("POP from empty stack");
        creg[instr.dst] = value_stack.peek();
        value_stack.pop();
        pc += 1;
        continue;
      }
      if (instr.op == OpCode::ArgLoad) {
        if (instr.dst >= num_bits) throw std::runtime_error("ARG dst out of range");
        if (frame_base_stack.empty()) throw std::runtime_error("ARG with no frame");
        long long base = frame_base_stack.peek();
        long long idx = instr.imm;
        if (idx < 0) throw std::runtime_error("ARG index < 0");
        long long pos = base + idx;
        if (pos < 0) throw std::runtime_error("ARG computed index < 0");
        if (static_cast<size_t>(pos) >= value_stack.size())
            throw std::runtime_error("ARG index out of range for value stack");
        creg[instr.dst] = value_stack.at(static_cast<size_t>(pos));
        pc += 1;
        continue;
      }
      if (instr.op == OpCode::Call) {
        long long target_pc = instr.imm;
        long long nargs = instr.imm2;
        if (target_pc < 0) throw std::runtime_error("CALL target < 0");
        if (nargs < 0) throw std::runtime_error("CALL nargs < 0");
        // push return pc (next instruction)
        return_pc_stack.push(pc + 1);

        // frame base points to where args begin on the value stack
        long long base = static_cast<long long>(value_stack.size()) - nargs;
        if (base < 0) throw std::runtime_error("CALL stack underflow for args");
        frame_base_stack.push(base);

        pc = target_pc;
        continue;
      }
      if (instr.op == OpCode::Ret) {
        long long nargs = instr.imm;
        if (nargs < 0) throw std::runtime_error("RET nargs < 0");
        if (return_pc_stack.empty() || frame_base_stack.empty())
          throw std::runtime_error("RET with empty call stack");

        // Return value is expected to be on top of value_stack.
        if (value_stack.empty()) throw std::runtime_error("RET missing return value");
        long long retv = value_stack.peek();
        value_stack.pop();

        // Pop args (nargs) + clear frame base record,no locals are stored; if locals added later, use another count
        for (long long k = 0; k < nargs; ++k) {
          if (value_stack.empty()) throw std::runtime_error("RET stack underflow");
          value_stack.pop();
        }

        frame_base_stack.pop();

        // Push return value back for caller
        value_stack.push(retv);

        pc = return_pc_stack.peek();
        return_pc_stack.pop();
        continue;
      }

      // quantum execution
      if (instr.op == OpCode::Init) {
        if (instr.target >= num_qubits)
          throw std::runtime_error("INIT qubit out of range");
        reset_qubit(state, instr.target);
        locked[instr.target] = 0;
        pc += 1;
        continue;
      }

      if (instr.op == OpCode::Measure) {
        if (instr.target >= num_qubits)
          throw std::runtime_error("MEASURE qubit out of range");
        if (instr.bit >= num_bits)
          throw std::runtime_error("MEASURE bit out of range");
        size_t result = 0;
        measure_qubit(state, instr.target, result, rng);

        classical_bits[instr.bit] = result;
        creg[instr.bit] = static_cast<long long>(result); // keep regs in sync

        locked[instr.target] = 1;
        pc += 1;
        continue;
      }

      if (instr.op == OpCode::Gate) {
        if (instr.target >= num_qubits)
          throw std::runtime_error("Gate target out of range");
        if (locked[instr.target])
          throw std::runtime_error("Gate applied to measured qubit without INIT");
        if (instr.control && *instr.control >= num_qubits)
          throw std::runtime_error("Gate control out of range");
        if (instr.control && locked[*instr.control])
          throw std::runtime_error("Gate control is a measured qubit without INIT");

        Matrix gate = resolve_gate(instr.gate, instr.parameter);
        if (instr.control)
          state.apply_gate(gate, instr.target, instr.control);
        else
          state.apply_gate(gate, instr.target, std::nullopt);

        pc += 1;
        continue;
      }

      if (instr.op == OpCode::End) {
        break;
      }

      throw std::runtime_error("Unknown opcode during execution");
    }

    size_t outcome = encode_bits(classical_bits);
    if (histogram.contains(outcome))
      histogram.at(outcome) += 1;
    else
      histogram.insert(outcome, 1);

    last_state = state;
    has_last_state = true;
  }
}

const Statevector &Backend::get_statevector() const { // resulting statevector
  if (!has_last_state)
    throw std::runtime_error("Statevector not available (run not called)");
  return last_state;
}

const HashTable<size_t, size_t> &
Backend::get_histogram() const { // resulting histogram
  return histogram;
}

size_t Backend::shots() const { return num_shots; }
size_t Backend::qubits() const { return num_qubits; }
size_t Backend::bits() const { return num_bits; }

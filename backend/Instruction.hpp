#ifndef INSTRUCTION_HPP
#define INSTRUCTION_HPP

#include <cstddef>
#include <optional>

enum class GateID { // individual gates
  I,
  H,
  X,
  Y,
  Z,
  S,
  T,
  S_DAG,
  T_DAG, // basic one qubit gates
  RX,
  RY,
  RZ,
  PHASE, // parameterized gates
  CNOT,
  CZ // two qubit controlled gates
};

enum class OpCode { // operation codes
  Gate,
  Measure,
  Init,

  // Classical VM ops
  // Use either dst/src/src2 for register indices, imm for immediates
  MovImm, // dst = imm
  Mov,    // dst = src1
  Add,    // dst = src1 + src2
  Sub,    // dst = src1 - src2
  Mul,    // dst = src1 * src2
  Div,    // dst = src1 / src2  (throws on div by 0)

  // Comparisons produce either 0 or 1 in dst
  CmpEq,  // dst = (src1 == src2)
  CmpNe,  // dst = (src1 != src2)
  CmpLt,  // dst = (src1 <  src2)
  CmpLe,  // dst = (src1 <= src2)
  CmpGt,  // dst = (src1 >  src2)
  CmpGe,  // dst = (src1 >= src2)

  // Control flow (pc-relative jumps)
  Jmp,        // pc += imm
  JmpIfZero,  // if (creg[src1] == 0) pc += imm
  JmpIfNonZero, // if (creg[src1] != 0) pc += imm

  End
};

struct Instruction {               // individual instructions
  OpCode op = OpCode::Gate;        // default opcode is a gate
  GateID gate = GateID::I;         // default instruction is an Identity
  std::optional<size_t> control;   // optional control qubit
  std::optional<double> parameter; // optional parameterized gate
  size_t target = 0;               // target qubit
  size_t bit = 0;                  // target bit (for measurement)

  // Classical fields, used when Op is a classical OpCode
  size_t dst = 0;
  size_t src1 = 0;
  size_t src2 = 0;
  long long imm = 0;
};

#endif

#include "Lower.hpp"
#include "QEmitter.hpp" // for make_gate1/make_gate2
#include <cstddef>

struct QRegInfo {
  size_t base = 0;
  size_t size = 0;
};

struct BRegInfo {
  size_t base = 0;
  size_t size = 0;
};

static void push_err(LowerResult& r, const char* msg) {
  r.ok = false;
  r.errors.push_back(String(msg));
}

static const Function* find_main(const Program& p) { // find any functions named main as an entry point
  for (size_t i = 0; i < p.functions.get_size(); ++i) {
    if (p.functions[i].name == String("main"))
      return &p.functions[i];
  }
  return nullptr;
}

static bool resolve_qubit_operand(const Operand& op, const HashTable<String, QRegInfo>& qregs, size_t& out_abs, LowerResult& r) { // handle qubit operand
  if (!qregs.contains(op.base)) {
    push_err(r, "Unknown qreg name in qubit operand");
    return false;
  }
  const QRegInfo& info = qregs.at(op.base);
  if (op.index >= info.size) {
    push_err(r, "Qubit index out of range for qreg");
    return false;
  }
  out_abs = info.base + op.index;
  return true;
}

static bool resolve_bit_operand(const Operand& op, const HashTable<String, BRegInfo>& bregs, size_t& out_abs, LowerResult& r) { // handle a bit
  if (!bregs.contains(op.base)) {
    push_err(r, "Unknown breg name in bit operand");
    return false;
  }
  const BRegInfo& info = bregs.at(op.base);
  if (op.index >= info.size) {
    push_err(r, "Bit index out of range for breg");
    return false;
  }
  out_abs = info.base + op.index;
  return true;
}

static void emit_gate1(LowerResult& r, const char* name, size_t a) { // push a gate of type gate1
  r.instr_lines.push_back(QEmitter::make_gate1(name, a));
}

static void emit_gate2(LowerResult& r, const char* name, size_t a, size_t b) { // push a gate with 2 params
  r.instr_lines.push_back(QEmitter::make_gate2(name, a, b));
}

LowerResult lower_to_q(const Program& p) { // lower fully down to .q format
  LowerResult r;

  const Function* main_fn = find_main(p); // main function
  if (!main_fn) {
    push_err(r, "No entry function func main() -> void found"); // error, fail bc no main function to run
    return r;
  }

  // Packed allocation state
  size_t next_qbase = 0;
  size_t next_cbase = 0;

  HashTable<String, QRegInfo> qregs;
  HashTable<String, BRegInfo> bregs;

  // First pass: process let qreg/breg declarations in order, allocate packed bases.
  for (size_t si = 0; si < main_fn->body.get_size(); ++si) {
    const Stmt& s = main_fn->body[si];
    if (s.kind != Stmt::Kind::LetReg) continue;

    const LetRegDecl& d = s.letreg; // register
    if (d.size == 0) {
      push_err(r, "Register size must be > 0");
      return r;
    }

    if (d.kind == LetRegDecl::Kind::QReg) { // qubit register
      if (qregs.contains(d.name) || bregs.contains(d.name)) {
        push_err(r, "Duplicate register name");
        return r;
      }
      QRegInfo info;
      info.base = next_qbase;
      info.size = d.size;
      qregs.insert(d.name, info);
      next_qbase += d.size;
    } else { // bit register
      if (qregs.contains(d.name) || bregs.contains(d.name)) {
        push_err(r, "Duplicate register name");
        return r;
      }
      BRegInfo info;
      info.base = next_cbase;
      info.size = d.size;
      bregs.insert(d.name, info);
      next_cbase += d.size;
    }
  }

  r.num_qubits = next_qbase;
  r.num_bits = next_cbase;

  if (r.num_qubits == 0) {
    push_err(r, "No qreg declared in main; cannot infer QUBITS"); // need to know amount of qubits from having registers
    return r;
  }
  if (r.num_bits == 0) {
    push_err(r, "No breg declared in main; cannot infer BITS"); // same with bits
    return r;
  }

  // Second pass: lower gate statements in order.
  for (size_t si = 0; si < main_fn->body.get_size(); ++si) {
    const Stmt& s = main_fn->body[si];

    if (s.kind == Stmt::Kind::Empty || s.kind == Stmt::Kind::LetReg) // ignore already passed
      continue;

    if (s.kind != Stmt::Kind::Gate) {
      push_err(r, "Unsupported statement in main (v1 circuit subset)");
      return r;
    }

    const GateStmt& g = s.gate;

    auto need_ops = [&](size_t n) -> bool {
      if (g.ops.get_size() != n) {
        push_err(r, "Wrong number of operands for gate");
        return false;
      }
      return true;
    };

    // 1-qubit gates
    if (g.kind == GateStmtKind::H || g.kind == GateStmtKind::X ||
        g.kind == GateStmtKind::Y || g.kind == GateStmtKind::Z ||
        g.kind == GateStmtKind::S || g.kind == GateStmtKind::T) {

      if (!need_ops(1)) return r;
      size_t q = 0;
      if (!resolve_qubit_operand(g.ops[0], qregs, q, r)) return r;

      const char* name =
          (g.kind == GateStmtKind::H) ? "H" :
          (g.kind == GateStmtKind::X) ? "X" :
          (g.kind == GateStmtKind::Y) ? "Y" :
          (g.kind == GateStmtKind::Z) ? "Z" :
          (g.kind == GateStmtKind::S) ? "S" : "T";

      emit_gate1(r, name, q);
      continue;
    }

    // controlled 2-qubit gates (control first, target second)
    if (g.kind == GateStmtKind::CX || g.kind == GateStmtKind::CNOT || g.kind == GateStmtKind::CZ) {
      if (!need_ops(2)) return r;

      size_t qc = 0, qt = 0;
      if (!resolve_qubit_operand(g.ops[0], qregs, qc, r)) return r;
      if (!resolve_qubit_operand(g.ops[1], qregs, qt, r)) return r;

      const char* name = (g.kind == GateStmtKind::CZ) ? "CZ" : "CNOT"; // CX => CNOT
      emit_gate2(r, name, qc, qt);
      continue;
    }

    // SWAP lowered to 3 CNOTs
    if (g.kind == GateStmtKind::SWAP) {
      if (!need_ops(2)) return r;

      size_t a = 0, b = 0;
      if (!resolve_qubit_operand(g.ops[0], qregs, a, r)) return r;
      if (!resolve_qubit_operand(g.ops[1], qregs, b, r)) return r;

      // CNOT a b; CNOT b a; CNOT a b;
      emit_gate2(r, "CNOT", a, b);
      emit_gate2(r, "CNOT", b, a);
      emit_gate2(r, "CNOT", a, b);
      continue;
    }

    // MEASURE qubit bit
    if (g.kind == GateStmtKind::MEASURE) {
      if (!need_ops(2)) return r;

      size_t q = 0, c = 0;
      if (!resolve_qubit_operand(g.ops[0], qregs, q, r)) return r;
      if (!resolve_bit_operand(g.ops[1], bregs, c, r)) return r;

      emit_gate2(r, "MEASURE", q, c);
      continue;
    }

    // RESET qubit => INIT qubit
    if (g.kind == GateStmtKind::RESET) {
      if (!need_ops(1)) return r;

      size_t q = 0;
      if (!resolve_qubit_operand(g.ops[0], qregs, q, r)) return r;

      emit_gate1(r, "INIT", q);
      continue;
    }

    push_err(r, "Unsupported gate in v1 lowering");
    return r;
  }

  return r;
}


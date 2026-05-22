#include "Lower.hpp"
#include "QEmitter.hpp" // for make_gate1/make_gate2
#include <cstddef>
#include <cstdio>
#include <cstdlib>

struct QRegInfo {
  size_t base = 0;
  size_t size = 0;
};

struct BRegInfo {
  size_t base = 0;
  size_t size = 0;
};

struct ScalarInfo { // for primitive types not register
  ScalarType type;
  size_t reg; // classical register index
};

static void push_err(LowerResult& r, const char* msg) {
  r.ok = false;
  r.errors.push_back(String(msg));
}

static String fmt3(const char* op, size_t a, size_t b, size_t c) {
  char buf[128];
  std::snprintf(buf, sizeof(buf), "%s %zu %zu %zu", op, a, b, c);
  return String(buf);
}

static String fmt2i(const char* op, size_t a, long long imm) {
  char buf[128];
  std::snprintf(buf, sizeof(buf), "%s %zu %lld", op, a, imm);
  return String(buf);
}

static String fmt2(const char* op, size_t a, size_t b) {
  char buf[128];
  std::snprintf(buf, sizeof(buf), "%s %zu %zu", op, a, b);
  return String(buf);
}

static String fmt1i(const char* op, long long imm) {
  char buf[128];
  std::snprintf(buf, sizeof(buf), "%s %lld", op, imm);
  return String(buf);
}

// Format a parameterized gate line, e.g. "RX 1.5707 2"
static String fmt_param_gate(const char* name, double param, size_t target) {
  char buf[256];
  std::snprintf(buf, sizeof(buf), "%s %g %zu", name, param, target);
  return String(buf);
}

static const Function* find_main(const Program& p) { // find any functions named main as an entry point
  for (size_t i = 0; i < p.functions.get_size(); ++i) {
    if (p.functions[i].name == String("main"))
      return &p.functions[i];
  }
  return nullptr;
}

// Find any function by name (for call lowering)
static const Function* find_function(const Program& p, const String& name) {
  for (size_t i = 0; i < p.functions.get_size(); ++i) {
    if (p.functions[i].name == name)
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

static void emit_classical(LowerResult& r, const String& line) {
  r.instr_lines.push_back(line);
}

// Forward declaration so lower_expr can call lower_stmt_list indirectly via call lowering
static bool lower_stmt_list(LowerResult& r,
                            const DynamicArray<Stmt>& body,
                            const HashTable<String, QRegInfo>& qregs,
                            const HashTable<String, BRegInfo>& bregs,
                            HashTable<String, ScalarInfo>& scalars,
                            size_t& next_temp_reg);

// Forward declaration of the full program pointer so call lowering can find functions
static const Program* g_program = nullptr;

// expression lowering, returns the classical register index containing the expression result
static size_t lower_expr(LowerResult& r,
                         const Expr* e,
                         HashTable<String, ScalarInfo>& scalars,
                         size_t& next_temp_reg) {
  if (!e) { push_err(r, "Null expression"); return 0; }

  // literals
  if (e->kind == ExprKind::IntLit) {
    size_t dst = next_temp_reg++;
    emit_classical(r, fmt2i("MOVI", dst, e->int_value));
    return dst;
  }
  if (e->kind == ExprKind::BoolLit) {
    size_t dst = next_temp_reg++;
    emit_classical(r, fmt2i("MOVI", dst, e->bool_value ? 1 : 0));
    return dst;
  }
  if (e->kind == ExprKind::Var) {
    if (!scalars.contains(e->var_name)) {
      push_err(r, "Unknown scalar variable");
      return 0;
    }
    return scalars.at(e->var_name).reg;
  }

  // Function call lowering using PUSH/CALL/POP
  if (e->kind == ExprKind::Call) {
    if (!g_program) { push_err(r, "Call expression with no program context"); return 0; }

    const Function* callee = find_function(*g_program, e->call_name);
    if (!callee) { push_err(r, "Call to unknown function"); return 0; }

    // push each argument onto the value stack
    long long nargs = static_cast<long long>(e->call_args.get_size());
    for (size_t ai = 0; ai < e->call_args.get_size(); ++ai) {
      size_t arg_reg = lower_expr(r, e->call_args[ai], scalars, next_temp_reg);
      emit_classical(r, fmt2("PUSH", 0, arg_reg)); // PUSH src — reuse fmt2 with dummy dst=0
      // PUSH uses src1 field; the line format is "PUSH <reg>"
      // fmt2 would emit "PUSH 0 arg_reg" which is wrong, use a dedicated formatter:
      {
        char buf[128];
        std::snprintf(buf, sizeof(buf), "PUSH %zu", arg_reg);
        // overwrite the last line we just pushed with the correct one
        r.instr_lines[r.instr_lines.get_size() - 1] = String(buf);
      }
    }


    // Save caller scalars, create fresh scope for callee
    HashTable<String, ScalarInfo> callee_scalars;

    // Bind parameters: pop from value_stack in reverse (last arg on top)
    // For inline lowering the args are already in regs; bind by position.
    for (size_t pi = 0; pi < callee->params.get_size(); ++pi) {
      size_t arg_reg = lower_expr(r, e->call_args[pi], scalars, next_temp_reg);
      ScalarInfo si;
      si.type = (callee->params[pi].type.kind == TypeKind::Bool) ? ScalarType::Bool : ScalarType::Int;
      si.reg = arg_reg;
      callee_scalars.insert(callee->params[pi].name, si);
    }

    // Allocate a register for the return value
    size_t ret_reg = next_temp_reg++;
    emit_classical(r, fmt2i("MOVI", ret_reg, 0)); // default 0

    // Lower callee body; Return stmts will MOV into ret_reg.
    // Store ret_reg in a known temp scalar name that Return lowering will detect.
    ScalarInfo ret_info;
    ret_info.type = ScalarType::Int;
    ret_info.reg = ret_reg;
    callee_scalars.insert(String("__ret__"), ret_info);

    // share the global qubit/bit space, so pass empty tables (no new regs declared inside a non-main function for now).
    HashTable<String, QRegInfo> empty_qregs;
    HashTable<String, BRegInfo> empty_bregs;
    lower_stmt_list(r, callee->body, empty_qregs, empty_bregs, callee_scalars, next_temp_reg);

    {
      (void)nargs; // suppress warning
    }

    return ret_reg;
  }

  // binary ops
  size_t a = lower_expr(r, e->left, scalars, next_temp_reg);
  size_t b = lower_expr(r, e->right, scalars, next_temp_reg);
  size_t dst = next_temp_reg++;

  switch (e->kind) {
    case ExprKind::Add: emit_classical(r, fmt3("ADD", dst, a, b)); break;
    case ExprKind::Sub: emit_classical(r, fmt3("SUB", dst, a, b)); break;
    case ExprKind::Mul: emit_classical(r, fmt3("MUL", dst, a, b)); break;
    case ExprKind::Div: emit_classical(r, fmt3("DIV", dst, a, b)); break;

    case ExprKind::Eq: emit_classical(r, fmt3("CMPEQ", dst, a, b)); break;
    case ExprKind::Ne: emit_classical(r, fmt3("CMPNE", dst, a, b)); break;
    case ExprKind::Lt: emit_classical(r, fmt3("CMPLT", dst, a, b)); break;
    case ExprKind::Le: emit_classical(r, fmt3("CMPLE", dst, a, b)); break;
    case ExprKind::Gt: emit_classical(r, fmt3("CMPGT", dst, a, b)); break;
    case ExprKind::Ge: emit_classical(r, fmt3("CMPGE", dst, a, b)); break;

    default:
      push_err(r, "Unsupported expression kind in lowering");
      break;
  }

  return dst;
}

// lower a single stmt
static bool lower_stmt(LowerResult& r,
                       const Stmt& s,
                       const HashTable<String, QRegInfo>& qregs,
                       const HashTable<String, BRegInfo>& bregs,
                       HashTable<String, ScalarInfo>& scalars,
                       size_t& next_temp_reg) {
  if (s.kind == Stmt::Kind::Empty || s.kind == Stmt::Kind::LetReg)
    return true;

  // Return stmt — move return expr result into __ret__ if present
  if (s.kind == Stmt::Kind::Return) {
    if (s.ret_expr && scalars.contains(String("__ret__"))) {
      size_t val = lower_expr(r, s.ret_expr, scalars, next_temp_reg);
      size_t ret_reg = scalars.at(String("__ret__")).reg;
      emit_classical(r, fmt2("MOV", ret_reg, val));
    }
    // void return or no __ret__ context: nothing to emit
    return r.ok;
  }

  if (s.kind == Stmt::Kind::LetScalar) {
    // allocate scalar to a classical reg
    if (scalars.contains(s.letscalar.name)) {
      push_err(r, "Duplicate scalar name");
      return false;
    }

    size_t reg = next_temp_reg++; // allocate from temp space, post-bits
    ScalarInfo info;
    info.type = s.letscalar.type;
    info.reg = reg;
    scalars.insert(s.letscalar.name, info);

    // init
    size_t val = lower_expr(r, s.letscalar.init, scalars, next_temp_reg);
    emit_classical(r, fmt2("MOV", reg, val));
    return r.ok;
  }

  if (s.kind == Stmt::Kind::If) {
    // cond -> reg
    size_t cond_reg = lower_expr(r, s.ifs.cond, scalars, next_temp_reg);

    // JZ cond <offset_to_else_or_end>
    // We'll patch offset once we know then-body length.
    size_t jz_pos = r.instr_lines.get_size();
    emit_classical(r, String("JZ 0 0")); // placeholder, patched later

    // then
    if (!lower_stmt_list(r, s.ifs.then_body, qregs, bregs, scalars, next_temp_reg))
      return false;

    bool has_else = (s.ifs.else_body.get_size() != 0);
    size_t jmp_pos = 0;
    if (has_else) {
      jmp_pos = r.instr_lines.get_size();
      emit_classical(r, String("JMP 0")); // placeholder
    }

    // compute jz offset: from jz instruction to else-start (or end)
    size_t after_then = r.instr_lines.get_size();
    size_t else_start = after_then;

    if (has_else) {
      // else starts immediately after JMP placeholder
      else_start = after_then;
      if (!lower_stmt_list(r, s.ifs.else_body, qregs, bregs, scalars, next_temp_reg))
        return false;
    }

    size_t after_else = r.instr_lines.get_size();

    // patch JZ
    // pc-relative in backend: pc += imm if condition met
    // if cond==0, jump to else_start
    // When executing JZ at pc=jz_pos, offset = (else_start - jz_pos)
    long long jz_off = static_cast<long long>(else_start) - static_cast<long long>(jz_pos);
    r.instr_lines[jz_pos] = fmt2i("JZ", cond_reg, jz_off);

    // patch JMP, skip else
    if (has_else) {
      long long jmp_off = static_cast<long long>(after_else) - static_cast<long long>(jmp_pos);
      r.instr_lines[jmp_pos] = fmt1i("JMP", jmp_off);
    }

    return r.ok;
  }

  if (s.kind == Stmt::Kind::Gate) {
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

      if (!need_ops(1)) return false;
      size_t q = 0;
      if (!resolve_qubit_operand(g.ops[0], qregs, q, r)) return false;

      const char* name =
          (g.kind == GateStmtKind::H) ? "H" :
          (g.kind == GateStmtKind::X) ? "X" :
          (g.kind == GateStmtKind::Y) ? "Y" :
          (g.kind == GateStmtKind::Z) ? "Z" :
          (g.kind == GateStmtKind::S) ? "S" : "T";

      emit_gate1(r, name, q);
      return true;
    }

    // Parameterized 1-qubit gates (RX, RY, RZ, PHASE)
    if (g.kind == GateStmtKind::RX || g.kind == GateStmtKind::RY ||
        g.kind == GateStmtKind::RZ || g.kind == GateStmtKind::PHASE) {

      if (!need_ops(1)) return false;
      if (!g.has_param) { push_err(r, "Parameterized gate missing parameter"); return false; }

      size_t q = 0;
      if (!resolve_qubit_operand(g.ops[0], qregs, q, r)) return false;

      const char* name =
          (g.kind == GateStmtKind::RX)    ? "RX"    :
          (g.kind == GateStmtKind::RY)    ? "RY"    :
          (g.kind == GateStmtKind::RZ)    ? "RZ"    : "PHASE";

      r.instr_lines.push_back(fmt_param_gate(name, g.param, q));
      return true;
    }

    // controlled 2-qubit gates (control first, target second)
    if (g.kind == GateStmtKind::CX || g.kind == GateStmtKind::CNOT || g.kind == GateStmtKind::CZ) {
      if (!need_ops(2)) return false;

      size_t qc = 0, qt = 0;
      if (!resolve_qubit_operand(g.ops[0], qregs, qc, r)) return false;
      if (!resolve_qubit_operand(g.ops[1], qregs, qt, r)) return false;

      const char* name = (g.kind == GateStmtKind::CZ) ? "CZ" : "CNOT"; // CX => CNOT
      emit_gate2(r, name, qc, qt);
      return true;
    }

    // SWAP lowered to 3 CNOTs
    if (g.kind == GateStmtKind::SWAP) {
      if (!need_ops(2)) return false;

      size_t a = 0, b = 0;
      if (!resolve_qubit_operand(g.ops[0], qregs, a, r)) return false;
      if (!resolve_qubit_operand(g.ops[1], qregs, b, r)) return false;

      emit_gate2(r, "CNOT", a, b);
      emit_gate2(r, "CNOT", b, a);
      emit_gate2(r, "CNOT", a, b);
      return true;
    }

    // MEASURE qubit bit
    if (g.kind == GateStmtKind::MEASURE) {
      if (!need_ops(2)) return false;

      size_t q = 0, c = 0;
      if (!resolve_qubit_operand(g.ops[0], qregs, q, r)) return false;
      if (!resolve_bit_operand(g.ops[1], bregs, c, r)) return false;

      emit_gate2(r, "MEASURE", q, c);
      return true;
    }

    // RESET qubit => INIT qubit
    if (g.kind == GateStmtKind::RESET) {
      if (!need_ops(1)) return false;

      size_t q = 0;
      if (!resolve_qubit_operand(g.ops[0], qregs, q, r)) return false;

      emit_gate1(r, "INIT", q);
      return true;
    }

    push_err(r, "Unsupported gate in lowering");
    return false;
  }

  push_err(r, "Unsupported statement in lowering");
  return false;
}

static bool lower_stmt_list(LowerResult& r,
                            const DynamicArray<Stmt>& body,
                            const HashTable<String, QRegInfo>& qregs,
                            const HashTable<String, BRegInfo>& bregs,
                            HashTable<String, ScalarInfo>& scalars,
                            size_t& next_temp_reg) {
  for (size_t si = 0; si < body.get_size(); ++si) {
    if (!lower_stmt(r, body[si], qregs, bregs, scalars, next_temp_reg))
      return false;
  }
  return true;
}

LowerResult lower_to_q(const Program& p) { // lower fully down to .q format
  LowerResult r;

  g_program = &p; // Set global program pointer for call lowering

  const Function* main_fn = find_main(p); // main function
  if (!main_fn) {
    push_err(r, "No entry function func main() -> void found");
    return r;
  }

  // Packed allocation state
  size_t next_qbase = 0;
  size_t next_cbase = 0;

  HashTable<String, QRegInfo> qregs;
  HashTable<String, BRegInfo> bregs;

  // Scalar symbol table (maps name to classical reg index)
  HashTable<String, ScalarInfo> scalars;

  // First pass: allocate qregs and bregs
  for (size_t si = 0; si < main_fn->body.get_size(); ++si) {
    const Stmt& s = main_fn->body[si];
    if (s.kind != Stmt::Kind::LetReg) continue;

    const LetRegDecl& d = s.letreg;
    if (d.size == 0) {
      push_err(r, "Register size must be > 0");
      return r;
    }

    if (d.kind == LetRegDecl::Kind::QReg) {
      if (qregs.contains(d.name) || bregs.contains(d.name)) {
        push_err(r, "Duplicate register name");
        return r;
      }
      QRegInfo info;
      info.base = next_qbase;
      info.size = d.size;
      qregs.insert(d.name, info);
      next_qbase += d.size;
    } else {
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

  // BITS now represents the size of the backend classical register file, includes breg bits (measurement targets), scalar variables, temporaries used during lowering
  size_t temp_base = next_cbase;
  size_t next_temp_reg = temp_base;

  // Second pass: lower all statements (including lets/ifs/gates) into .q lines
  if (!lower_stmt_list(r, main_fn->body, qregs, bregs, scalars, next_temp_reg))
    return r;

  // finalize classical reg count
  r.num_bits = next_temp_reg;

  if (r.num_qubits == 0) {
    push_err(r, "No qreg declared in main; cannot infer QUBITS");
    return r;
  }
  if (r.num_bits == 0) {
    push_err(r, "No classical regs allocated; cannot infer BITS");
    return r;
  }

  g_program = nullptr; // clear global pointer when done
  return r;
}

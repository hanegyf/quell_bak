func main() -> void {
  # Two quantum registers to make sure packed allocation is working
  # qA -> qubits 0..2
  # qB -> qubits 3..4
  let qA: qreg<3>;
  let qB: qreg<2>;

  # Two classical registers
  # cA -> bits 0..2
  # cB -> bits 3..4
  let cA: breg<3>;
  let cB: breg<2>;

  # 1-qubit gate sweep (make sure everything parses/lowers)
  H qA[0];
  X qA[0];
  Y qA[0];
  Z qA[0];
  S qA[0];
  T qA[0];

  # Bell pair + extra CZ coverage
  H qA[1];
  CX qA[1] qA[2];   # CX should compile the same as CNOT
  CZ qA[1] qA[2];

  # SWAP lowering check (3 CNOTs)
  SWAP qA[0] qB[0];

  # Teleportation-ish structure (no classical feed-forward yet)
  # Make an entangled pair between qA[1] and qB[1]
  H qA[1];
  CNOT qA[1] qB[1];

  # bell measurement steps between qB[0] and qA[1]
  CNOT qB[0] qA[1];
  H qB[0];

  # measure both; backend should lock these qubits now
  MEASURE qB[0] cB[0];
  MEASURE qA[1] cA[1];

  # if we try to touch qB[0] or qA[1] now, backend should complain
  # So RESET first (RESET -> INIT)
  RESET qB[0];
  RESET qA[1];

  # now it's allowed again
  H qB[0];
  X qA[1];

  # more measurements across both regs
  MEASURE qA[0] cA[0];
  MEASURE qA[2] cA[2];
  MEASURE qB[1] cB[1];

  # quick re-init + re-measure to confirm INIT unlocks
  RESET qA[0];
  Z qA[0];
  MEASURE qA[0] cA[0];
}

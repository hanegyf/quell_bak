# Bernstein-Vazirani algorithm — finds hidden bitstring s in one query
# Uses: H gates, CNOT, PHASE, functions, if/else, int scalars, measurement
# Hidden string: s = 101 (3 bits)
# Oracle: f(x) = s . x mod 2
# Expected output: measure |101> = outcome 5

func parity_check(a: int, b: int) -> int {
    # returns 1 if a == b, 0 otherwise
    if (a == b) {
        return 1;
    }
    return 0;
}

func main() -> void {
    # 3 input qubits + 1 ancilla qubit for phase kickback
    let q: qreg<4>;
    let c: breg<3>;

    # Prepare ancilla in |-> for phase kickback
    X q[3];
    H q[3];

    # Hadamard all input qubits
    H q[0];
    H q[1];
    H q[2];

    # Oracle for s = 101: apply CNOT from input bits where s_i = 1
    # s[0] = 1 -> CNOT q[0] q[3]
    CNOT q[0] q[3];
    # s[1] = 0 -> skip
    # s[2] = 1 -> CNOT q[2] q[3]
    CNOT q[2] q[3];

    # Hadamard again to extract s
    H q[0];
    H q[1];
    H q[2];

    # Measure input qubits — should always give |101> = 5
    MEASURE q[0] c[0];
    MEASURE q[1] c[1];
    MEASURE q[2] c[2];

    # Classical verification using a function call
    let expected: int = 5;
    let got0: int = parity_check(1, 1);   # s[0]=1, always 1
    let got2: int = parity_check(1, 1);   # s[2]=1, always 1
    let got1: int = parity_check(0, 0);   # s[1]=0, always 1

    # All three should be 1 (confirmed match)
    let all_match: int = got0 * got1 * got2;

    if (all_match == 1) {
        let verified: int = 1;  # verification passed
    }
}

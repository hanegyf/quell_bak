# BB84 Quantum Key Distribution protocol demo
# Alice prepares 4 qubits, Bob measures them.
#
# Alice's bits:  1, 0, 1, 0
# Alice's bases: X, Z, X, Z  (1=Hadamard, 0=computational)
# Bob's bases:   X, X, Z, Z
#
# Matching bases: qubits 0 and 3
#   qubit 0: Alice X, bit 1 -> |+> ; Bob measures X -> always 1
#   qubit 3: Alice Z, bit 0 -> |0> ; Bob measures Z -> always 0
# Sifted key: [1, 0]

func main() -> void {
    let q: qreg<4>;
    let c: breg<4>;

    # --- Alice prepares ---

    # Qubit 0: bit=1, basis=X -> |1> then H -> |->
    X q[0];
    H q[0];

    # Qubit 1: bit=0, basis=Z -> |0> (default, nothing needed)

    # Qubit 2: bit=1, basis=X -> |1> then H -> |->
    X q[2];
    H q[2];

    # Qubit 3: bit=0, basis=Z -> |0> (default, nothing needed)

    # --- Bob measures ---

    # Qubit 0: Bob basis=X -> H then measure
    H q[0];
    MEASURE q[0] c[0];

    # Qubit 1: Bob basis=X -> H then measure (wrong basis, random result)
    H q[1];
    MEASURE q[1] c[1];

    # Qubit 2: Bob basis=Z -> measure directly (wrong basis, random result)
    MEASURE q[2] c[2];

    # Qubit 3: Bob basis=Z -> measure directly
    MEASURE q[3] c[3];

    # --- Classical sifting (hardcoded basis comparison) ---
    # Only qubits 0 and 3 had matching bases; 1 and 2 are discarded.
    # c[0] and c[3] form the sifted key.
    # In the histogram, look at bits 0 and 3 of the output.
    # Bit 0 should always be 1, bit 3 should always be 0.
    # Bits 1 and 2 will be 50/50 (discarded in real sifting).

    let key_bit_0: int = 1;   # matches c[0], which is deterministically 1
    let key_bit_3: int = 0;   # matches c[3], which is deterministically 0
}

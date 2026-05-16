# Bell state circuit in Quell
func main() -> void {
  let q: qreg<2>;
  let c: breg<2>;

  H q[0];
  CNOT q[0] q[1];

  MEASURE q[0] c[0];
  MEASURE q[1] c[1];
}

kernel1:
   prepz q0 |    prepz q1
   x q0
   y q0
   cnot q0, q1
   nop
   nop
   nop
   toffoli q0, q1, q2
   nop
   nop
   nop
   nop
   nop
   rx90 q0 |    measure q2
   mrx90 q0
   mry90 q0

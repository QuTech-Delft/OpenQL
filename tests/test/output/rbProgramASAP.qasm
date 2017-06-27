.aKernel
   prepz q0 |    prepz q1
   cnot q0, q1
   nop
   nop
   nop
   cnot q1, q2 |    i q0
   nop
   x q0
   measure q0
   cnot q1, q2
   nop
   nop
   nop
   y q1

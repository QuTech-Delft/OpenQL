qubits 5

.aKernel
    { x q1 | x q0 }
    qwait 7
    swap q0,q1
    qwait 15
    { cz q1,q4 | cz q0,q2 }
    qwait 15


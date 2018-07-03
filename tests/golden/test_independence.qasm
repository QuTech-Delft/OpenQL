qubits 4
.aKernel

    { prepz q0 | prepz q1 | prepz q2 | prepz q3 }
    qwait 7
    { cz q0,q1 | cz q2,q3 }
    qwait 7
    { measure q0 | measure q1 }
    qwait 59


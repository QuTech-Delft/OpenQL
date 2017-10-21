.aKernel
prepz q0 | prepz q1 | prepz q2 | prepz q3
qwait 7
cz q0,q1
qwait 7
cz q1,q2 | measure q0
qwait 15
measure q1

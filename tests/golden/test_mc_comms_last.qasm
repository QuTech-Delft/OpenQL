version 1.0
# this file has been automatically generated by the OpenQL compiler please do not modify it manually.
qubits 16

.kernel_comms
    x q[0]
    move q[0],q[1]
    wait 7
    x q[4]
    tmove q[4],q[0]
    wait 29
    cnot q[1],q[0]
    wait 4

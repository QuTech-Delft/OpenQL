# Generated by OpenQL 0.11.1 for program test_mc_noncomms
version 1.2

pragma @ql.name("test_mc_noncomms")


.kernel_noncomms
    x q[7]
    tmove q[7], q[0]
    skip 38
    x q[3]
    cnot q[3], q[0]
    skip 4

# Generated by OpenQL 0.9.0 for program test_mc_locals
version 1.2

pragma @ql.name("test_mc_locals")


.kernel_locals
    { # start at cycle 0
        x q[0]
        x q[4]
        x q[8]
        x q[12]
    }
    { # start at cycle 1
        cnot q[0], q[1]
        cnot q[4], q[5]
        cnot q[8], q[9]
        cnot q[12], q[13]
    }
    skip 4
    { # start at cycle 6
        x q[1]
        x q[5]
        x q[9]
        x q[13]
    }
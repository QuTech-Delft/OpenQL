# Generated by OpenQL 0.11.1 for program test_mapper_allNN
version 1.2

pragma @ql.name("test_mapper_allNN")


.kernel_allNN
    { # start at cycle 0
        x q[0]
        x q[3]
    }
    cnot q[0], q[3]
    skip 2
    x q[1]
    cnot q[1], q[3]
    skip 2
    { # start at cycle 8
        x q[2]
        x q[5]
    }
    { # start at cycle 9
        cnot q[0], q[2]
        cnot q[3], q[5]
    }
    skip 3
    cnot q[3], q[0]
    skip 2
    { # start at cycle 16
        x q[4]
        x q[6]
    }
    { # start at cycle 17
        cnot q[1], q[4]
        cnot q[3], q[6]
    }
    skip 3
    { # start at cycle 21
        cnot q[3], q[1]
        cnot q[2], q[5]
        cnot q[4], q[6]
    }
    skip 3
    cnot q[5], q[3]
    skip 3
    { # start at cycle 29
        cnot q[2], q[0]
        cnot q[4], q[1]
        cnot q[6], q[3]
    }
    skip 3
    { # start at cycle 33
        cnot q[5], q[2]
        cnot q[6], q[4]
    }
    skip 3
    { # start at cycle 37
        x q[0]
        x q[1]
        x q[2]
        x q[3]
        x q[4]
        x q[5]
        x q[6]
    }
# Generated by OpenQL 0.11.1 for program test_mapper_allIP
version 1.2

pragma @ql.name("test_mapper_allIP")


.kernel_allIP
    { # start at cycle 0
        x q[1]
        x q[3]
    }
    swap q[1], q[3]
    skip 2
    { # start at cycle 4
        x q[2]
        x q[5]
    }
    swap q[2], q[5]
    skip 4
    x q[0]
    cnot q[0], q[3]
    skip 3
    cnot q[3], q[5]
    skip 3
    x q[3]
    swap q[3], q[1]
    skip 9
    cnot q[5], q[3]
    skip 3
    { # start at cycle 34
        x q[4]
        x q[6]
        x q[5]
    }
    { # start at cycle 35
        swap q[4], q[6]
        swap q[5], q[2]
    }
    skip 9
    cnot q[3], q[6]
    skip 3
    x q[3]
    swap q[3], q[6]
    skip 3
    swap q[1], q[4]
    skip 5
    cnot q[3], q[5]
    skip 3
    x q[3]
    swap q[3], q[1]
    skip 9
    cnot q[5], q[3]
    skip 3
    { # start at cycle 79
        x q[0]
        x q[3]
        x q[5]
    }
# Generated by OpenQL 0.9.0 for program test_edge_ALAP
version 1.2

pragma @ql.name("test_edge_ALAP")


.kernel_edge_ALAP
    cz q[1], q[4]
    skip 3
    { # start at cycle 4
        cz q[0], q[3]
        cz q[2], q[5]
    }
    skip 3
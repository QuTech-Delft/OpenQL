# Generated by OpenQL 0.9.0 for program test_cz_NN_any_commute
version 1.2

pragma @ql.name("test_cz_NN_any_commute")


.aKernel
    cz q[0], q[3]
    skip 1
    cz q[3], q[6]
    skip 1
    { # start at cycle 4
        t q[6]
        cz q[1], q[3]
    }
    skip 1
    t q[1]
    z q[6]
    skip 1
    z q[1]
    skip 1
    t q[1]
    skip 2
    z q[1]
    skip 1
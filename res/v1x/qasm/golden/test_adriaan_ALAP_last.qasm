# Generated by OpenQL 0.9.0 for program test_adriaan_ALAP
version 1.2

pragma @ql.name("test_adriaan_ALAP")


.kernel_adriaan_ALAP
    prepz q[0]
    skip 1
    x q[0]
    skip 1
    x q[0]
    skip 1
    x q[0]
    skip 1
    { # start at cycle 8
        prepz q[2]
        x q[0]
    }
    skip 1
    { # start at cycle 10
        x q[0]
        rx90 q[2]
    }
    skip 1
    { # start at cycle 12
        x q[0]
        rx90 q[2]
    }
    skip 1
    { # start at cycle 14
        x q[0]
        rx90 q[2]
    }
    skip 1
    { # start at cycle 16
        x q[0]
        rx90 q[2]
    }
    skip 1
    { # start at cycle 18
        x q[0]
        rx90 q[2]
    }
    skip 1
    { # start at cycle 20
        x q[0]
        rx90 q[2]
    }
    skip 1
    { # start at cycle 22
        measure q[2]
        measure q[0]
    }
    skip 1
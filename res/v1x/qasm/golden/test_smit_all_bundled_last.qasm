# Generated by OpenQL 0.9.0 for program test_smit_all_bundled
version 1.2

pragma @ql.name("test_smit_all_bundled")


.aKernel
    { # start at cycle 0
        prepz q[0]
        prepz q[1]
        prepz q[2]
        prepz q[3]
        prepz q[4]
        prepz q[5]
        prepz q[6]
    }
    skip 1
    { # start at cycle 2
        cz q[2], q[0]
        cz q[3], q[5]
        cz q[1], q[4]
    }
    skip 3
    { # start at cycle 6
        cz q[4], q[6]
        cz q[2], q[5]
        cz q[3], q[0]
    }
    skip 3
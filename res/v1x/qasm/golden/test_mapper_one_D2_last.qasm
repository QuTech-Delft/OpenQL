# Generated by OpenQL 0.11.1 for program test_mapper_one_D2
version 1.2

pragma @ql.name("test_mapper_one_D2")


.kernel_one_D2
    y q[3]
    swap q[3], q[0]
    skip 9
    x q[2]
    cnot q[2], q[0]
    skip 3
    { # start at cycle 16
        y q[0]
        x q[2]
    }
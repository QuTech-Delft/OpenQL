# Generated by OpenQL 0.11.1 for program test_mc_all_saturate
version 1.2

pragma @ql.name("test_mc_all_saturate")


.kernel_all_saturate
    x q[12]
    { # start at cycle 1
        x q[8]
        cnot q[12], q[13]
    }
    cnot q[8], q[9]
    skip 3
    x q[13]
    tswap q[13], q[8]
    skip 52
    x q[0]
    cnot q[0], q[1]
    skip 4
    x q[1]
    tswap q[8], q[1]
    skip 7
    x q[4]
    cnot q[4], q[5]
    skip 4
    x q[5]
    { # start at cycle 82
        cnot q[2], q[3]
        tmove q[4], q[11]
        tmove q[5], q[15]
    }
    skip 4
    tmove q[3], q[10]
    skip 34
    tswap q[15], q[11]
    skip 4
    tswap q[1], q[10]
    skip 15
    tmove q[0], q[14]
    skip 38
    cnot q[8], q[11]
    { # start at cycle 183
        cnot q[6], q[7]
        cnot q[14], q[15]
    }
    skip 3
    { # start at cycle 187
        cnot q[11], q[9]
        cnot q[8], q[10]
    }
    { # start at cycle 188
        cnot q[7], q[4]
        cnot q[2], q[0]
        cnot q[14], q[12]
        cnot q[6], q[5]
        cnot q[15], q[13]
        cnot q[1], q[3]
    }
    skip 3
    cnot q[8], q[9]
    { # start at cycle 193
        cnot q[2], q[3]
        cnot q[6], q[4]
        cnot q[7], q[5]
        cnot q[14], q[13]
        cnot q[15], q[12]
        cnot q[1], q[0]
        cnot q[11], q[10]
    }
    skip 3
    x q[9]
    { # start at cycle 198
        cnot q[7], q[6]
        cnot q[4], q[5]
        cnot q[15], q[14]
        cnot q[13], q[12]
        cnot q[1], q[2]
        cnot q[11], q[8]
        cnot q[9], q[10]
        cnot q[3], q[0]
    }
    skip 4
    { # start at cycle 203
        cnot q[4], q[7]
        cnot q[5], q[6]
        cnot q[13], q[15]
        cnot q[12], q[14]
        cnot q[3], q[1]
        cnot q[9], q[11]
        cnot q[10], q[8]
        cnot q[0], q[2]
    }
    skip 4
    { # start at cycle 208
        cnot q[4], q[6]
        cnot q[5], q[7]
        cnot q[13], q[14]
        cnot q[12], q[15]
        cnot q[9], q[8]
        cnot q[3], q[2]
        cnot q[10], q[11]
        cnot q[0], q[1]
    }
    skip 4
    { # start at cycle 213
        cnot q[5], q[4]
        cnot q[12], q[13]
        cnot q[0], q[3]
        cnot q[10], q[9]
    }
    skip 4
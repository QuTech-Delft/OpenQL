# File:         test_cc.py
# Purpose:      test the central controller backend
# Based on:     ../test_hybrid.py, ../test_uniform_sched.py

import os
import unittest
from openql import openql as ql

rootDir = os.path.dirname(os.path.realpath(__file__))
curdir = os.path.dirname(__file__)
output_dir = os.path.join(curdir, 'test_output')
config_fn = os.path.join(curdir, 'test_cfg_cc.json')
platform_name = 's-17'
num_qubits = 25
num_cregs = 32


class Test_central_controller(unittest.TestCase):

    @classmethod
    def setUpClass(self):
        ql.set_option('output_dir', output_dir)
        ql.set_option('optimize', 'no')
        ql.set_option('scheduler', 'ALAP')
        ql.set_option('log_level', 'LOG_DEBUG')

    def test_classical(self):
        platform = ql.Platform(platform_name, config_fn)

        p = ql.Program('test_classical', platform, num_qubits, num_cregs)
        k1 = ql.Kernel('aKernel1', platform, num_qubits, num_cregs)

        # quantum operations
        k1.gate('x', [6])
        k1.gate('cz', [6, 7])

        # create classical registers
        rd = ql.CReg()
        rs1 = ql.CReg()
        rs2 = ql.CReg()

        if 0:
            # add/sub/and/or/xor
            k1.classical(rd, ql.Operation(rs1, '+', rs2))

            # not
            k1.classical(rd, ql.Operation('~', rs2))

            # comparison
            k1.classical(rd, ql.Operation(rs1, '==', rs2))

            # initialize (r1 = 2)
            k1.classical(rs1, ql.Operation(2))

            # assign (r1 = r2)
            k1.classical(rs1, ql.Operation(rs2))

        # measure
        k1.gate('measure', [6], rs1)
        k1.gate('measure', [7], rs2)

        # add kernel
        p.add_kernel(k1)
        p.compile()

    def test_uniform_scheduler_0(self):
        ql.set_option('output_dir', output_dir)
        ql.set_option('optimize', 'no')
        ql.set_option('scheduler', 'ALAP')
        ql.set_option('scheduler_uniform', 'yes')
        ql.set_option('log_level', 'LOG_WARNING')

        platform = ql.Platform(platform_name, config_fn)

        p = ql.Program('test_uniform_scheduler_0', platform, num_qubits, num_cregs)
        k = ql.Kernel('kernel_0', platform, num_qubits, num_cregs)

        # a simple first test
        # the x gates serve to separate the cnot gates wrt dependences
        # this creates big bundles with 7 x gates
        # and small bundles with just a cnot
        # after uniform scheduling, one or more x gates
        # should have been moved next to the cnot
        # those will move that do not have operands that overlap those of the cnot

        for j in range(6, 18+1):
            k.gate("x", [j])

        k.gate("cnot", [6, 7])

        for j in range(6, 18+1):
            k.gate("x", [j])

        k.gate("cnot", [12, 13])

        for j in range(6, 18+1):
            k.gate("x", [j])

        k.gate("cnot", [10, 15])

        p.add_kernel(k)
        p.compile()

    # Quantum Error Correction cycle
    def test_qec(self):
        ql.set_option('output_dir', output_dir)
        ql.set_option('optimize', 'no')
        ql.set_option('scheduler', 'ALAP')
        ql.set_option('scheduler_uniform', 'yes')
        ql.set_option('log_level', 'LOG_WARNING')

        platform = ql.Platform(platform_name, config_fn)

        p = ql.Program('test_qec', platform, num_qubits, num_cregs)
        k = ql.Kernel('kernel_0', platform, num_qubits, num_cregs)


        k.gate("cnot", [6, 7])
        # FIXME: add proper code

        p.add_kernel(k)
        p.compile()

    # FIXME: add:
    # - qec_pipelined
    # - nested loops
    # - long program (RB)


if __name__ == '__main__':
    unittest.main()

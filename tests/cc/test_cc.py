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
num_qubits = 17
num_cregs = 32
all_qubits = range(0, num_qubits)


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

    # Quantum Error Correction cycle
    @unittest.skip
    def test_qec(self):
        ql.set_option('output_dir', output_dir)
        ql.set_option('optimize', 'no')
        ql.set_option('scheduler', 'ALAP')
        ql.set_option('scheduler_uniform', 'yes')
        ql.set_option('log_level', 'LOG_WARNING')

        platform = ql.Platform(platform_name, config_fn)

        p = ql.Program('test_qec', platform, num_qubits, num_cregs)
        k = ql.Kernel('kernel_0', platform, num_qubits, num_cregs)

        # pipelined QEC: [
        # see: R. Versluis et al., Phys. Rev. A 8, 034021 (2017)
        # - nw, ne, sw, se] -> [n, e, w, s] because we rotate grid
        # - H -> rym90, ry90, see Fig 2 of reference
        #
        # class SurfaceCode, qubits, tiles, width, getNeighbourN, getNeighbourE, getNeighbourW, getNeighbourS, getX, getZ, getData

        # define qubit aliases:
        # FIXME: neighbours make no sense anymore
        x = 7
        xN = x-5
        xE = x+1
        xS = x+5
        xW = x-1

        z = 11
        zN = z-5
        zE = z+1
        zS = z+5
        zW = z-1

        # create classical registers
        rdX = ql.CReg()
        rdZ = ql.CReg()

        # X stabilizers
        k.gate("rym90", [x])
        k.gate("rym90", [xN])
        k.gate("rym90", [xE])
        k.gate("rym90", [xW])
        k.gate("rym90", [xS])
        k.wait(all_qubits, 0)
#        k.wait({x, xN, xE, xW, xS}, 0)

        k.gate("cz", [x, xE])
        k.gate("cz", [x, xN])
        k.gate("cz", [x, xS])
        k.gate("cz", [x, xW])
        k.wait(all_qubits, 0)
#        k.wait({x, xN, xE, xW, xS}, 0)

        k.gate("ry90", [x])
        k.gate("ry90", [xN])
        k.gate("ry90", [xE])
        k.gate("ry90", [xW])
        k.gate("ry90", [xS])
        k.wait(all_qubits, 0)
#        k.wait({x, xN, xE, xW, xS}, 0)

        k.gate("measure", [x], rdX)
#        k.wait(all_qubits, 0)
        k.wait([x], 0)

        # Z stabilizers
        k.gate("rym90", [z])

        k.gate("cz", [z, zE])
        k.gate("cz", [z, zS])
        k.gate("cz", [z, zN])
        k.gate("cz", [z, zW])

        k.gate("ry90", [z])
        k.gate("measure", [z], rdZ)

        p.add_kernel(k)
        p.compile()

    def test_angle(self):
        ql.set_option('output_dir', output_dir)
        ql.set_option('optimize', 'no')
        ql.set_option('scheduler', 'ALAP')
        ql.set_option('scheduler_uniform', 'yes')
        ql.set_option('log_level', 'LOG_WARNING')

        platform = ql.Platform(platform_name, config_fn)

        p = ql.Program('test_angle', platform, num_qubits, num_cregs)
        k = ql.Kernel('kernel_0', platform, num_qubits, num_cregs)

        k.gate("rx180", [6], 0, 1.2345)     # NB: Python interface lacks classical parameter

        p.add_kernel(k)
        p.compile()

    def test_qi_example(self):
        ql.set_option('output_dir', output_dir)
        ql.set_option('optimize', 'no')
        ql.set_option('scheduler', 'ALAP')
        ql.set_option('scheduler_uniform', 'yes')
        ql.set_option('log_level', 'LOG_WARNING')

        platform = ql.Platform(platform_name, os.path.join(curdir, 'cc_s5_direct_iq.json'))

        p = ql.Program('test_qi_example', platform, 5, num_cregs)
        k = ql.Kernel('kernel_0', platform, 5, num_cregs)

        k.gate("prepz", [0, 1, 2, 3, 4])
        k.gate("ry180", [0, 2])     # FIXME: "y" does not work, but gate decomposition should handle?
        k.gate("cz", [0, 2])
        k.gate("y90", [2])
        k.gate("measure", [0, 1, 2, 3, 4])

        p.add_kernel(k)
        p.compile()

    def test_gate_decomposition_builtin_gates(self):
        ql.set_option('output_dir', output_dir)
        ql.set_option('optimize', 'no')
        ql.set_option('scheduler', 'ALAP')
        ql.set_option('scheduler_uniform', 'yes')
        ql.set_option('log_level', 'LOG_DEBUG')
        #ql.set_option('log_level', 'LOG_WARNING')

        platform = ql.Platform(platform_name, os.path.join(curdir, 'cc_s5_direct_iq.json'))

        p = ql.Program('test_gate_decomposition_builtin_gates', platform, 5, num_cregs)
        k = ql.Kernel('kernel_0', platform, 5, num_cregs)

        k.gate("cz", [0, 2])
        k.gate("cz", [2, 3])
        k.gate("cz", [3, 2])
        k.gate("cz", [2, 4])
        k.gate("cz", [4, 2])

        p.add_kernel(k)
        p.compile()



    # FIXME: add:
    # - qec_pipelined
    # - nested loops
    # - long program (RB)



if __name__ == '__main__':
    unittest.main()

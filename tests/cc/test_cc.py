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
num_bregs = 32
all_qubits = range(0, num_qubits)


class Test_central_controller(unittest.TestCase):

    @classmethod
    def setUp(self):
        ql.initialize()
        ql.set_option('output_dir', output_dir)
        ql.set_option('optimize', 'no')
        ql.set_option('scheduler', 'ALAP')
        ql.set_option('scheduler_uniform', 'yes')
        ql.set_option('log_level', 'LOG_WARNING')

    def test_classical(self):
        platform = ql.Platform(platform_name, config_fn)

        p = ql.Program('test_classical', platform, num_qubits, num_cregs, num_bregs)
        k = ql.Kernel('aKernel1', platform, num_qubits, num_cregs, num_bregs)

        # quantum operations
        k.gate('x', [6])
        k.gate('cz', [6, 7])

        # create classical registers
        if 0:   # FIXME: deprecated by branch condex
            rd = ql.CReg(1)
            rs1 = ql.CReg(2)
            rs2 = ql.CReg(3)

        if 0:
            # add/sub/and/or/xor
            k.classical(rd, ql.Operation(rs1, '+', rs2))

            # not
            k.classical(rd, ql.Operation('~', rs2))

            # comparison
            k.classical(rd, ql.Operation(rs1, '==', rs2))

            # initialize (r1 = 2)
            k.classical(rs1, ql.Operation(2))

            # assign (r1 = r2)
            k.classical(rs1, ql.Operation(rs2))

        # measure
        k.barrier([])
        k.gate('measure', [6], 0,0.0, [0])
        k.gate('measure', [7], 0,0.0, [1])

        # add kernel
        p.add_kernel(k)
        p.compile()

    # Quantum Error Correction cycle
    def test_qec(self):
        platform = ql.Platform(platform_name, config_fn)

        p = ql.Program('test_qec', platform, num_qubits, num_cregs, num_bregs)
        k = ql.Kernel('kernel_0', platform, num_qubits, num_cregs, num_bregs)

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
        if 0:      # FIXME: deprecated by branch condex
            rdX = ql.CReg(1)
            rdZ = ql.CReg(2)

        # X stabilizers
        k.gate("rym90", [x])
        k.gate("rym90", [xN])
        k.gate("rym90", [xE])
        k.gate("rym90", [xW])
        k.gate("rym90", [xS])
        k.barrier([])

        k.gate("cz", [x, xE])
        k.gate("cz", [x, xN])
        k.gate("cz", [x, xS])
        k.gate("cz", [x, xW])
        k.barrier([])

        k.gate("ry90", [x])
        k.gate("ry90", [xN])
        k.gate("ry90", [xE])
        k.gate("ry90", [xW])
        k.gate("ry90", [xS])
        k.barrier([])

#        k.gate("measure", [x], rdX)
        k.gate('measure', [x], 0,0.0, [0])
        k.barrier([])

        # Z stabilizers
        k.gate("rym90", [z])
        k.barrier([])

        k.gate("cz", [z, zE])
        k.gate("cz", [z, zS])
        k.gate("cz", [z, zN])
        k.gate("cz", [z, zW])
        k.barrier([])

        k.gate("ry90", [z])
        # k.gate("measure", [z], rdZ)
        k.gate('measure', [z], 0,0.0, [1])

        p.add_kernel(k)
        p.compile()

    def test_angle(self):
        platform = ql.Platform(platform_name, config_fn)

        p = ql.Program('test_angle', platform, num_qubits, num_cregs, num_bregs)
        k = ql.Kernel('kernel_0', platform, num_qubits, num_cregs, num_bregs)

        k.gate("rx180", [6], 0, 1.2345)     # NB: Python interface lacks classical parameter

        p.add_kernel(k)
        p.compile()

    def test_qi_example(self):
        platform = ql.Platform(platform_name, os.path.join(curdir, 'cc_s5_direct_iq.json'))

        p = ql.Program('test_qi_example', platform, 5, num_cregs, num_bregs)
        k = ql.Kernel('kernel_0', platform, 5, num_cregs, num_bregs)

        k.barrier([])
        for q in [0, 1, 2, 3, 4]:
            k.gate("prepz", [q])
        k.barrier([])

        k.gate("ry180", [0, 2])     # FIXME: "y" does not work, but gate decomposition should handle?
        k.gate("cz", [0, 2])
        k.gate("y90", [2])
        k.barrier([])
        for q in [0, 1, 2, 3, 4]:
            k.gate("measure", [q])
        k.barrier([])

        p.add_kernel(k)
        p.compile()

    def test_gate_decomposition_builtin_gates(self):
        platform = ql.Platform(platform_name, os.path.join(curdir, 'cc_s5_direct_iq.json'))

        p = ql.Program('test_gate_decomposition_builtin_gates', platform, 5, num_cregs, num_bregs)
        k = ql.Kernel('kernel_0', platform, 5, num_cregs, num_bregs)

        k.gate("cz", [0, 2])
        k.gate("cz", [2, 3])
        k.gate("cz", [3, 2])
        k.gate("cz", [2, 4])
        k.gate("cz", [4, 2])

        p.add_kernel(k)
        p.compile()

    # based on ../test_cqasm_reader.py::test_conditions
    def test_conditions(self):
        cqasm_config_fn = os.path.join(curdir, 'cqasm_config_cc.json')
        platform = ql.Platform(platform_name, os.path.join(curdir, 'cc_s5_direct_iq.json'))
        # platform = ql.Platform('seven_qubits_chip', config_fn)
        number_qubits = platform.get_qubit_number()
        name = 'test_cqasm_conditions'
        program = ql.Program(name, platform, number_qubits)
        qasm_rdr = ql.cQasmReader(platform, program, cqasm_config_fn)
        qasm_str = "version 1.1\n"                  \
                   "var qa, qb: qubit\n"            \
                   "var ca, cb: bool\n"             \
                   "measure qa, ca\n"               \
                   "measure qb, cb\n"               \
                   "cond(true) x qa\n"              \
                   "cond(false) y qa\n"             \
                   "cond(ca) z qa\n"                \
                   "cond(!true) x qa\n"             \
                   "cond(!false) y qa\n"            \
                   "cond(!ca) z qa\n"               \
                   "cond(!!true) x qa\n"            \
                   "cond(!!false) y qa\n"           \
                   "cond(!!ca) z qa\n"              \
                   "cond(ca && cb) x qa\n"          \
                   "cond(ca && true) y qa\n"        \
                   "cond(ca && false) z qa\n"       \
                   "cond(true && cb) x qa\n"        \
                   "cond(false && cb) y qa\n"       \
                   "cond(ca || cb) z qa\n"          \
                   "cond(ca || true) x qa\n"        \
                   "cond(ca || false) y qa\n"       \
                   "cond(true || cb) z qa\n"        \
                   "cond(false || cb) x qa\n"       \
                   "cond(ca ^^ cb) y qa\n"          \
                   "cond(ca ^^ true) z qa\n"        \
                   "cond(ca ^^ false) x qa\n"       \
                   "cond(true ^^ cb) y qa\n"        \
                   "cond(false ^^ cb) z qa\n"       \
                   "cond(!(ca && cb)) x qa\n"       \
                   "cond(!(ca && true)) y qa\n"     \
                   "cond(!(ca && false)) z qa\n"    \
                   "cond(!(true && cb)) x qa\n"     \
                   "cond(!(false && cb)) y qa\n"    \
                   "cond(!(ca || cb)) z qa\n"       \
                   "cond(!(ca || true)) x qa\n"     \
                   "cond(!(ca || false)) y qa\n"    \
                   "cond(!(true || cb)) z qa\n"     \
                   "cond(!(false || cb)) x qa\n"    \
                   "cond(!(ca ^^ cb)) y qa\n"       \
                   "cond(!(ca ^^ true)) z qa\n"     \
                   "cond(!(ca ^^ false)) x qa\n"    \
                   "cond(!(true ^^ cb)) y qa\n"     \
                   "cond(!(false ^^ cb)) z qa\n"
        qasm_rdr.string2circuit(qasm_str)
        program.compile()
        #self.assertTrue(file_compare(os.path.join(output_dir, name + '.qasm'), os.path.join(curdir, 'golden', name + '.qasm')))




    # FIXME: add:
    # - qec_pipelined
    # - nested loops
    # - long program (RB)



if __name__ == '__main__':
    unittest.main()

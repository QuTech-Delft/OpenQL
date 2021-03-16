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
    def test_cqasm_conditions(self):
        cqasm_config_fn = os.path.join(curdir, 'cqasm_config_cc.json')
        platform = ql.Platform(platform_name, os.path.join(curdir, 'cc_s5_direct_iq.json'))
        number_qubits = platform.get_qubit_number()
        name = 'test_cqasm_conditions'
        program = ql.Program(name, platform, number_qubits)
        qasm_rdr = ql.cQasmReader(platform, program, cqasm_config_fn)
        qasm_str = """
            version 1.1
            
            var qa, qb: qubit
            var ca, cb: bool
            
            { measure_fb qa, ca | measure_fb qb, cb }
            # .barrier  # use subcircuit to force new kernel, and thus scheduling realm 
            # barrier [qa,qb]
            # { barrier qa | barrier qb }
            barrier
            
            # # Old hack for feedback
            # { measure qa, ca | measure qb, cb }
            # 
            # .wait_uhfqa # use subcircuit to force new kernel, and thus scheduling realm
            # { _wait_uhfqa qa | _wait_uhfqa qb }
            # .dist_dsm
            # { _dist_dsm qa, ca | _dist_dsm qb, cb }
            # .wait_dsm
            # { _wait_dsm qa | _wait_dsm qb }        
            # .test
            #             
            cond(true) x qa
            cond(false) y qa
            cond(ca) z qa
            cond(!true) x qa
            cond(!false) y qa
            cond(!ca) z qa
            cond(!!true) x qa
            cond(!!false) y qa
            cond(!!ca) z qa
            cond(ca && cb) x qa
            cond(ca && true) y qa
            cond(ca && false) z qa
            cond(true && cb) x qa
            cond(false && cb) y qa
            cond(ca || cb) z qa
            cond(ca || true) x qa
            cond(ca || false) y qa
            cond(true || cb) z qa
            cond(false || cb) x qa
            cond(ca ^^ cb) y qa
            cond(ca ^^ true) z qa
            cond(ca ^^ false) x qa
            cond(true ^^ cb) y qa
            cond(false ^^ cb) z qa
            cond(!(ca && cb)) x qa
            cond(!(ca && true)) y qa
            cond(!(ca && false)) z qa
            cond(!(true && cb)) x qa
            cond(!(false && cb)) y qa
            cond(!(ca || cb)) z qa
            cond(!(ca || true)) x qa
            cond(!(ca || false)) y qa
            cond(!(true || cb)) z qa
            cond(!(false || cb)) x qa
            cond(!(ca ^^ cb)) y qa
            cond(!(ca ^^ true)) z qa
            cond(!(ca ^^ false)) x qa
            cond(!(true ^^ cb)) y qa
            cond(!(false ^^ cb)) z qa
            """
        qasm_rdr.string2circuit(qasm_str)
        ql.set_option('log_level', 'LOG_INFO')
        program.compile()
        #self.assertTrue(file_compare(os.path.join(output_dir, name + '.qasm'), os.path.join(curdir, 'golden', name + '.qasm')))


    def test_cqasm_gate_decomposition(self):
        cqasm_config_fn = os.path.join(curdir, 'cqasm_config_cc.json')
        platform = ql.Platform(platform_name, os.path.join(curdir, 'cc_s5_direct_iq.json'))
        number_qubits = platform.get_qubit_number()
        name = 'test_cqasm_gate_decomposition'
        program = ql.Program(name, platform, number_qubits)
        qasm_rdr = ql.cQasmReader(platform, program, cqasm_config_fn)
        qasm_str = """
            version 1.1
            
            var qa, qb: qubit
            var ca, cb: bool
            
            { measure_fb qa, ca | measure_fb qb, cb }
            """
        qasm_rdr.string2circuit(qasm_str)
        ql.set_option('log_level', 'LOG_INFO')
        program.compile()

    # based on test_hybrid.py::test_do_while_nested_for()
    def test_nested_rus(self):
        qidx = 0

        platform = ql.Platform(platform_name, os.path.join(curdir, 'cc_s5_direct_iq.json'))
        p = ql.Program('test_nested_rus', platform, 5, num_cregs, num_bregs)

        # FIXME: don't use underscores in program/kernel names if those are used as parameters to add_for()/add_do_while()
        # since incorrect labels are generated in that case (both in .qasm and .vq1asm files)
        outer_program = ql.Program('outerProgram', platform, 5, num_cregs, num_bregs)
        outer_kernel = ql.Kernel('outerKernel', platform, 5, num_cregs)
        outer_kernel.gate("measure_fb", [qidx])
        outer_kernel.gate("if_1_break", [qidx])
        outer_program.add_kernel(outer_kernel)

        inner_program = ql.Program('innerProgram', platform, 5, num_cregs, num_bregs)
        inner_kernel = ql.Kernel('innerKernel', platform, 5, num_cregs)
        inner_kernel.gate("measure_fb", [qidx])
        inner_kernel. gate("if_0_break", [qidx])
        inner_kernel.gate("rx180", [qidx])
        inner_program.add_for(inner_kernel, 1000000) # NB: loops *kernel* # NB: loops *kernel*

        outer_program.add_program(inner_program)

        foo = ql.CReg(0)
        p.add_do_while(outer_program, ql.Operation(foo, '==', foo)) # NB: loops *program*, CC backend interprets all conditions as true

        ql.set_option('log_level', 'LOG_INFO') # override log level
        p.compile()



    # FIXME: add:
    # - qec_pipelined
    # - long program (RB)



if __name__ == '__main__':
    unittest.main()

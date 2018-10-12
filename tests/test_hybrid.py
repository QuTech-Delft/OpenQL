import os
import unittest
from openql import openql as ql
from test_QISA_assembler_present import assemble

rootDir = os.path.dirname(os.path.realpath(__file__))

curdir = os.path.dirname(__file__)
output_dir = os.path.join(curdir, 'test_output')

ql.set_option('output_dir', output_dir)
ql.set_option('optimize', 'no')
ql.set_option('scheduler', 'ALAP')
ql.set_option('log_level', 'LOG_WARNING')

def file_compare(fn1, fn2):
    isSame = False
    with open(fn1, 'r') as f1:
        with open(fn2, 'r') as f2:
            a = f1.read()
            b = f2.read()
            f1.close()
            f2.close()
            isSame = (a==b)
    return isSame



class Test_hybrid_classical_quantum(unittest.TestCase):

    def test_classical(self):
        config_fn = os.path.join(curdir, 'hardware_config_cc_light.json')
        platform  = ql.Platform('seven_qubits_chip', config_fn)
        num_qubits = 5
        num_cregs = 10

        p = ql.Program('test_classical', platform, num_qubits, num_cregs)
        sweep_points = [1,2]
        p.set_sweep_points(sweep_points, len(sweep_points))

        k1 = ql.Kernel('aKernel1', platform, num_qubits, num_cregs)

        # quanutm operations
        k1.gate('x', [0])
        k1.gate('cz', [0, 2])

        # # create classical registers
        rd = ql.CReg()
        rs1 = ql.CReg()
        rs2 = ql.CReg()

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
        k1.gate('measure', [0], rs1)

        # add kernel
        p.add_kernel(k1)
        p.compile()

        QISA_fn = os.path.join(output_dir, p.name+'.qisa')
        assemble(QISA_fn)


    def test_if(self):
        config_fn = os.path.join(curdir, 'hardware_config_cc_light.json')
        platform  = ql.Platform('seven_qubits_chip', config_fn)
        num_qubits = 5
        num_cregs = 10

        p = ql.Program('test_if', platform, num_qubits, num_cregs)
        sweep_points = [1,2]
        p.set_sweep_points(sweep_points, len(sweep_points))

        k1 = ql.Kernel('aKernel1', platform, num_qubits, num_cregs)
        k2 = ql.Kernel('aKernel2', platform, num_qubits, num_cregs)

        # create classical registers
        rd = ql.CReg()
        rs1 = ql.CReg()
        rs2 = ql.CReg()

        # quanutm operations
        k1.gate('x', [0])
        k2.gate('y', [0])

        # simple if
        p.add_if(k1, ql.Operation(rs1, '==', rs2))
        p.add_kernel(k2)

        p.compile()

        QISA_fn = os.path.join(output_dir, p.name+'.qisa')
        assemble(QISA_fn)

    def test_if_else(self):
        config_fn = os.path.join(curdir, 'hardware_config_cc_light.json')
        platform  = ql.Platform('seven_qubits_chip', config_fn)
        num_qubits = 5
        num_cregs = 10

        p = ql.Program('test_if_else', platform, num_qubits, num_cregs)
        sweep_points = [1,2]
        p.set_sweep_points(sweep_points, len(sweep_points))

        k1 = ql.Kernel('aKernel1', platform, num_qubits, num_cregs)
        k2 = ql.Kernel('aKernel2', platform, num_qubits, num_cregs)

        # create classical registers
        rd = ql.CReg()
        rs1 = ql.CReg()
        rs2 = ql.CReg()

        # quanutm operations
        k1.gate('x', [0])
        k2.gate('y', [0])

        # simple if
        p.add_if_else(k1, k2, ql.Operation(rs1, '==', rs2))

        p.compile()

        QISA_fn = os.path.join(output_dir, p.name+'.qisa')
        assemble(QISA_fn)


    def test_for(self):
        config_fn = os.path.join(curdir, 'hardware_config_cc_light.json')
        platform  = ql.Platform('seven_qubits_chip', config_fn)
        num_qubits = 5
        num_cregs = 10

        p = ql.Program('test_for', platform, num_qubits, num_cregs)
        sweep_points = [1,2]
        p.set_sweep_points(sweep_points, len(sweep_points))

        k1 = ql.Kernel('aKernel1', platform, num_qubits, num_cregs)
        k2 = ql.Kernel('aKernel2', platform, num_qubits, num_cregs)

        # create classical registers
        rd = ql.CReg()
        rs1 = ql.CReg()
        rs2 = ql.CReg()

        # quanutm operations
        k1.gate('x', [0])
        k2.gate('y', [0])

        p.add_for(k1, 10)
        p.add_kernel(k2)

        p.compile()

        QISA_fn = os.path.join(output_dir, p.name+'.qisa')
        assemble(QISA_fn)

    def test_do_while(self):
        config_fn = os.path.join(curdir, 'hardware_config_cc_light.json')
        platform  = ql.Platform('seven_qubits_chip', config_fn)
        num_qubits = 5
        num_cregs = 10

        p = ql.Program('test_do_while', platform, num_qubits, num_cregs)
        sweep_points = [1,2]
        p.set_sweep_points(sweep_points, len(sweep_points))

        k1 = ql.Kernel('aKernel1', platform, num_qubits, num_cregs)
        k2 = ql.Kernel('aKernel2', platform, num_qubits, num_cregs)

        # create classical registers
        rd = ql.CReg()
        rs1 = ql.CReg()
        rs2 = ql.CReg()

        # quanutm operations
        k1.gate('x', [0])
        k2.gate('y', [0])

        p.add_do_while(k1, ql.Operation(rs1, '<', rs2))
        p.add_kernel(k2)

        p.compile()

        QISA_fn = os.path.join(output_dir, p.name+'.qisa')
        assemble(QISA_fn)

    def test_do_while_nested_for(self):
        config_fn = os.path.join(curdir, 'hardware_config_cc_light.json')
        platform  = ql.Platform('seven_qubits_chip', config_fn)
        num_qubits = 5
        num_cregs = 10

        p = ql.Program('test_do_while_nested_for', platform, num_qubits, num_cregs)
        sweep_points = [1,2]
        p.set_sweep_points(sweep_points, len(sweep_points))

        sp1 = ql.Program('subprogram1', platform, num_qubits, num_cregs)
        sp2 = ql.Program('subprogram2', platform, num_qubits, num_cregs)

        k1 = ql.Kernel('aKernel1', platform, num_qubits, num_cregs)
        k2 = ql.Kernel('aKernel2', platform, num_qubits, num_cregs)

        # create classical registers
        rd = ql.CReg()
        rs1 = ql.CReg()
        rs2 = ql.CReg()

        # quanutm operations
        k1.gate('x', [0])
        k2.gate('y', [0])

        sp1.add_do_while(k1, ql.Operation(rs1, '>=', rs2))
        sp2.add_for(sp1, 100)
        p.add_program(sp2)

        p.compile()

        QISA_fn = os.path.join(output_dir, p.name+'.qisa')
        assemble(QISA_fn)

if __name__ == '__main__':
    unittest.main()

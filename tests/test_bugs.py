import os
import filecmp
import unittest
import numpy as np
from openql import openql as ql
from utils import file_compare

rootDir = os.path.dirname(os.path.realpath(__file__))
curdir = os.path.dirname(__file__)
output_dir = os.path.join(curdir, 'test_output')

class Test_bugs(unittest.TestCase):
    def setUp(self):
        ql.set_option('output_dir', output_dir)

    # @unittest.expectedFailure
    # @unittest.skip
    def test_typecast(self):
        sweep_points = [1,2]
        num_circuits = 1
        num_qubits = 2
        config_fn = os.path.join(curdir, 'test_config_default.json')
        platf = ql.Platform("starmon", config_fn)
        p = ql.Program('test_bug', platf, num_qubits)
        p.set_sweep_points(sweep_points, len(sweep_points))
        k = ql.Kernel('kernel1', platf, num_qubits)

        qubit = 1

        k.identity(np.int(qubit))
        k.identity(np.int32(qubit))
        k.identity(np.int64(qubit))

        k.identity(np.uint(qubit))
        k.identity(np.uint32(qubit))
        k.identity(np.uint64(qubit))

        # add the kernel to the program
        p.add_kernel(k)



    def test_operation_order_190(self):
        config_fn = os.path.join(curdir, 'hardware_config_cc_light.json')
        platform = ql.Platform("myPlatform", config_fn)

        sweep_points = [1]
        nqubits = 4
        nregs = 4

        p1 = ql.Program("order1_prog", platform, nqubits, nregs)
        p2 = ql.Program("order2_prog", platform, nqubits, nregs)
        p1.set_sweep_points(sweep_points, len(sweep_points))
        p2.set_sweep_points(sweep_points, len(sweep_points))
        k1 = ql.Kernel("aKernel", platform, nqubits, nregs)
        k2 = ql.Kernel("aKernel", platform, nqubits, nregs)


        k1.gate('x', [0])
        k1.gate('y', [2])

        # gates are added in reverse order
        k2.gate('y', [2])
        k2.gate('x', [0])

        p1.add_kernel(k1)
        p2.add_kernel(k2)

        p1.compile()
        p2.compile()
        self.assertTrue( file_compare(
            os.path.join(output_dir, p1.name+'.qisa'),
            os.path.join(output_dir, p2.name+'.qisa')) )


    # relates to https://github.com/QE-Lab/OpenQL/issues/171
    # various runs of compiles were generating different results or in the best
    # case strange errors. So multiple (NCOMPILES) runs of compile are executed
    # to make sure there is no error and output generated in all these runs is same
    def test_stateful_behavior(self):
        ql.set_option('optimize', 'no')
        ql.set_option('scheduler', 'ALAP')
        ql.set_option('log_level', 'LOG_WARNING')

        config_fn = os.path.join(curdir, 'hardware_config_cc_light.json')
        platform = ql.Platform("myPlatform", config_fn)

        sweep_points = [1]
        nqubits = 3
        nregs = 3

        p = ql.Program("statelessProgram", platform, nqubits, nregs)
        p.set_sweep_points(sweep_points, len(sweep_points))
        k = ql.Kernel("aKernel", platform, nqubits, nregs)

        k.prepz(0)
        k.gate('rx180', [0])
        k.measure(0)

        rd = ql.CReg()
        rs1 = ql.CReg()
        rs2 = ql.CReg()

        k.classical(rs1, ql.Operation(3))
        k.classical(rs1, ql.Operation(4))
        k.classical(rd, ql.Operation(rs1, '+', rs2))

        p.add_kernel(k)

        NCOMPILES=50
        QISA_fn = os.path.join(output_dir, p.name+'.qisa')
        for i in range(NCOMPILES):
            p.compile()
            QISA_fn_i = os.path.join(output_dir, p.name+'_'+str(i)+'.qisa')
            os.rename(QISA_fn,QISA_fn_i)

        for i in range(NCOMPILES-1):
            QISA_fn_1 = os.path.join(output_dir, p.name+'_'+str(i)+'.qisa')
            QISA_fn_2 = os.path.join(output_dir, p.name+'_'+str(i+1)+'.qisa')
            self.assertTrue( file_compare(QISA_fn_1, QISA_fn_2))

if __name__ == '__main__':
    unittest.main()

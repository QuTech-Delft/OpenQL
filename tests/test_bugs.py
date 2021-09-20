import os
import filecmp
import unittest
import numpy as np
from openql import openql as ql
from utils import file_compare

curdir = os.path.dirname(os.path.realpath(__file__))
output_dir = os.path.join(curdir, 'test_output')

class Test_bugs(unittest.TestCase):
    @classmethod
    def setUp(self):
        ql.initialize()
        ql.set_option('output_dir', output_dir)
        ql.set_option('use_default_gates', 'yes')
        ql.set_option('log_level', 'LOG_WARNING')
        os.chdir(curdir) # test_empty_infinite_loop uses relative paths for test_output

    # @unittest.expectedFailure
    # @unittest.skip
    def test_typecast(self):
        sweep_points = [1,2]
        num_circuits = 1
        num_qubits = 2
        platf = ql.Platform("starmon", 'cc_light')
        p = ql.Program('test_bug', platf, num_qubits)
        p.set_sweep_points(sweep_points)
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


    # relates to https://github.com/QE-Lab/OpenQL/issues/171
    # various runs of compiles were generating different results or in the best
    # case strange errors. So multiple (NCOMPILES) runs of compile are executed
    # to make sure there is no error and output generated in all these runs is same
    # JvS: more likely, it also had to do with the classical register allocator
    # depending on stuff like Python's garbage collection to free a register.
    # The register numbers have to be hardcoded now for that reason.
    def test_stateful_behavior(self):
        ql.set_option('optimize', 'no')
        ql.set_option('scheduler', 'ALAP')

        platform = ql.Platform("myPlatform", 'cc_light')

        sweep_points = [1]
        nqubits = 3
        nregs = 3

        p = ql.Program("statelessProgram", platform, nqubits, nregs)
        p.set_sweep_points(sweep_points)
        k = ql.Kernel("aKernel", platform, nqubits, nregs)

        k.prepz(0)
        k.gate('rx180', [0])
        k.measure(0)

        rd = ql.CReg(0)
        rs1 = ql.CReg(1)
        rs2 = ql.CReg(2)

        k.classical(rs1, ql.Operation(3))
        k.classical(rs1, ql.Operation(4))
        k.classical(rd, ql.Operation(rs1, '+', rs2))

        p.add_kernel(k)

        NCOMPILES=50
        QISA_fn = os.path.join(output_dir, p.name+'_last.qasm')
        for i in range(NCOMPILES):
            p.compile()
            self.setUpClass()
            QISA_fn_i = os.path.join(output_dir, p.name+'_'+str(i)+'_last.qasm')
            os.rename(QISA_fn, QISA_fn_i)

        for i in range(NCOMPILES-1):
            QISA_fn_1 = os.path.join(output_dir, p.name+'_'+str(i)+'_last.qasm')
            QISA_fn_2 = os.path.join(output_dir, p.name+'_'+str(i+1)+'_last.qasm')
            self.assertTrue( file_compare(QISA_fn_1, QISA_fn_2))

    def test_empty_infinite_loop(self):
        name = 'empty_infinite_loop'
        in_fn = 'test_' + name + '.cq'
        out_fn = 'test_output/' + name + '_out.cq' # must match path set inside in_fn
        gold_fn = 'golden/' + name + '_out.cq'
        ql.initialize()  # note that output path is set inside file in_fn, so output_dir is not respected
        #ql.set_option('log_level', 'LOG_DEBUG')
        ql.compile(in_fn)
        self.assertTrue(file_compare(out_fn, gold_fn))

if __name__ == '__main__':
    unittest.main()

import numpy as np
import openql as ql
import os
import unittest

from config import cq_dir, cq_golden_dir, output_dir
from utils import file_compare


class TestBugs(unittest.TestCase):
    @classmethod
    def setUp(cls):
        ql.initialize()
        ql.set_option('output_dir', output_dir)
        ql.set_option('use_default_gates', 'yes')
        ql.set_option('log_level', 'LOG_WARNING')
        cur_dir = os.path.dirname(os.path.realpath(__file__))
        os.chdir(cur_dir)  # test_empty_infinite_loop uses relative paths for test_output

    # @unittest.expectedFailure
    # @unittest.skip
    def test_typecast(self):
        num_qubits = 2
        platform = ql.Platform("starmon", 'cc_light')
        p = ql.Program('test_bug', platform, num_qubits)
        k = ql.Kernel('kernel1', platform, num_qubits)

        qubit = 1

        k.identity(int(qubit))
        k.identity(np.int32(qubit))
        k.identity(np.int64(qubit))

        k.identity(np.uint(qubit))
        k.identity(np.uint32(qubit))
        k.identity(np.uint64(qubit))

        # add the kernel to the program
        p.add_kernel(k)

    # relates to https://github.com/QuTech-Delft/OpenQL/issues/171
    # various runs of compiles were generating different results or in the best
    # case strange errors. So multiple (num_compilations) runs of compile are executed
    # to make sure there is no error and output generated in all these runs is same
    # JvS: more likely, it also had to do with the classical register allocator
    # depending on stuff like Python's garbage collection to free a register.
    # The register numbers have to be hardcoded now for that reason.
    def test_stateful_behavior(self):
        ql.set_option('optimize', 'no')
        ql.set_option('scheduler', 'ALAP')

        platform = ql.Platform("myPlatform", 'cc_light')

        num_qubits = 3
        num_regs = 3

        p = ql.Program("statelessProgram", platform, num_qubits, num_regs)
        k = ql.Kernel("aKernel", platform, num_qubits, num_regs)

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

        num_compilations = 50
        qisa_fn = os.path.join(output_dir, p.name + '_last.qasm')
        for i in range(num_compilations):
            p.compile()
            self.setUpClass()
            qisa_fn_i = os.path.join(output_dir, p.name + '_' + str(i) + '_last.qasm')
            os.rename(qisa_fn, qisa_fn_i)

        for i in range(num_compilations - 1):
            qisa_fn_1 = os.path.join(output_dir, p.name + '_' + str(i) + '_last.qasm')
            qisa_fn_2 = os.path.join(output_dir, p.name + '_' + str(i + 1) + '_last.qasm')
            self.assertTrue(file_compare(qisa_fn_1, qisa_fn_2))

    def test_empty_infinite_loop(self):
        name = 'empty_infinite_loop'
        in_fn = os.path.join(cq_dir, 'test_' + name + '.cq')
        out_fn = os.path.join(output_dir, name + '_out.cq')  # must match path set inside in_fn
        gold_fn = os.path.join(cq_golden_dir, name + '_out.cq')
        ql.initialize()  # note that output path is set inside file in_fn, so output_dir is not respected
        # ql.set_option('log_level', 'LOG_DEBUG')
        ql.compile(in_fn)
        self.assertTrue(file_compare(out_fn, gold_fn))


if __name__ == '__main__':
    unittest.main()

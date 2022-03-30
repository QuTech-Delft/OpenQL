from openql import openql as ql
import unittest
import os
from utils import file_compare

curdir = os.path.dirname(os.path.realpath(__file__))
output_dir = os.path.join(curdir, 'test_output')


class Tester(unittest.TestCase):

    @classmethod
    def setUp(self):
        ql.initialize()
        ql.set_option('output_dir', output_dir)
        ql.set_option('optimize', 'no')
        ql.set_option('scheduler', 'ALAP')
        ql.set_option('log_level', 'LOG_DEBUG')

    def test_decomposition(self):
        config_fn = os.path.join(curdir, 'test_cfg_none.json')
        platform  = ql.Platform('platform_none', config_fn)
        num_qubits = 17
        p = ql.Program('test_decomposition', platform, num_qubits)

        k = ql.Kernel('aKernel', platform, num_qubits)

        k.x(0) # x will be decomposed
        k.gate("x", [0]); # x will be decomposed
        k.gate("y", [0]); # decomposition not available, will use custom gate
        k.gate("s", [1]); # decomposition as well as custom gate not available, will use default gate
        k.gate("roty90", [0]) # any name can be used for composite gate

        k.gate("cnot", [0, 1] ) # decomposition overrules custom(=primitive) gate; decomposition uses specialized cz q0,q1
        k.gate("cnot", [2, 3] ) # cnot will be decomposed; decomposition falls back to default gate for cz q2,q3

        # add the kernel to the program
        p.add_kernel(k)

        # compile the program
        p.compile()

        prog_name = "test_decomposition"
        gold_fn = curdir + '/golden/' + prog_name +'_scheduled.qasm'
        qasm_fn = os.path.join(output_dir, prog_name+'_scheduled.qasm')
        self.assertTrue(file_compare(qasm_fn, gold_fn))


if __name__ == '__main__':
    unittest.main()

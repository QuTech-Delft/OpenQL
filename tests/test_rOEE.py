
from openql import openql as ql
import os
import unittest
from utils import file_compare

curdir = os.path.dirname(os.path.realpath(__file__))
output_dir = os.path.join(curdir, 'test_output')

class Test_rOEE(unittest.TestCase):

    def setUp(self):
        ql.initialize()
        ql.set_option('output_dir', output_dir)
        ql.set_option('log_level', 'LOG_NOTHING')
        ql.set_option('use_default_gates', 'no')
        ql.set_option('optimize', 'no')
        ql.set_option('generate_code', 'no')
        ql.set_option('print_dot_graphs', 'no')

        ql.set_option('scheduler', 'ALAP')
        ql.set_option('maprOEE', 'yes')

    def test_rOEE(self):
        config = os.path.join(curdir, "test_multi_core_4x4_full.json")
        platform  = ql.Platform("mc4x4full", config)
        num_qubits = 16
        p = ql.Program('test_rOEE', platform, num_qubits)
        k = ql.Kernel('kernel_rOEE', platform, num_qubits)
        for i in range(4):
            k.gate("x", [4*i])
            k.gate("x", [4*i+1])
        for i in range(4):
            k.gate("cnot", [4*i,4*i+1])
        for i in range(4):
            for j in range(4):
                if i != j:
                    k.gate("cnot", [4*i,4*j])
                    k.gate("cnot", [4*i+1,4*j+1])
                    k.gate("cnot", [4*i+2,4*j+2])
                    k.gate("cnot", [4*i+3,4*j+3])

        p.add_kernel(k)
        p.compile()

        gold_fn = curdir + '/golden/' + 'test_rOEE' +'_last.qasm'
        qasm_fn = os.path.join(output_dir, 'test_rOEE'+'_last.qasm')
        self.assertTrue( file_compare(qasm_fn, gold_fn) )

if __name__ == '__main__':
    unittest.main()
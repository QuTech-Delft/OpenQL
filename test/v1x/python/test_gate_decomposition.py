import openql as ql
import os
import unittest

from config import json_dir, output_dir, qasm_golden_dir
from utils import file_compare


class Tester(unittest.TestCase):
    @classmethod
    def setUp(cls):
        ql.initialize()
        ql.set_option('output_dir', output_dir)
        ql.set_option('optimize', 'no')
        ql.set_option('scheduler', 'ALAP')
        ql.set_option('log_level', 'LOG_NOTHING')

    def test_decomposition(self):
        config_fn = os.path.join(json_dir, 'test_cfg_none.json')
        platform = ql.Platform('platform_none', config_fn)
        num_qubits = 17
        p = ql.Program('test_decomposition', platform, num_qubits)

        k = ql.Kernel('aKernel', platform, num_qubits)

        k.x(0)  # x will be decomposed
        k.gate("x", [0])  # x will be decomposed
        k.gate("y", [0])  # decomposition not available, will use custom gate
        k.gate("s", [1])  # decomposition as well as custom gate not available, will use default gate
        k.gate("roty90", [0])  # any name can be used for composite gate

        # decomposition overrules custom(=primitive) gate
        # decomposition uses specialized cz q0,q1
        k.gate("cnot", [0, 1])
        # cnot will be decomposed; decomposition falls back to default gate for cz q2,q3
        k.gate("cnot", [2, 3])

        # add the kernel to the program
        p.add_kernel(k)

        # compile the program
        p.compile()

        prog_name = "test_decomposition"
        gold_fn = os.path.join(qasm_golden_dir, prog_name + '_scheduled.qasm')
        qasm_fn = os.path.join(output_dir, prog_name + '_scheduled.qasm')
        self.assertTrue(file_compare(qasm_fn, gold_fn))


if __name__ == '__main__':
    unittest.main()

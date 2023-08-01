import openql as ql
import os
import unittest

from config import json_dir, output_dir, qasm_golden_dir
from utils import file_compare


class TestConjugatedKernel(unittest.TestCase):
    @classmethod
    def setUp(cls):
        ql.initialize()
        ql.set_option('output_dir', output_dir)
        ql.set_option('optimize', 'no')
        ql.set_option('scheduler', 'ASAP')
        ql.set_option('log_level', 'LOG_WARNING')
        # ql.set_option('write_qasm_files', 'yes')

    def test_conjugate(self):
        config_fn = os.path.join(json_dir, 'test_cfg_none_simple.json')
        platform = ql.Platform('platform_none', config_fn)
        num_qubits = 3
        p = ql.Program('test_conjugate', platform, num_qubits)

        k = ql.Kernel('kernel_original', platform, num_qubits)
        ck = ql.Kernel('kernel_conjugate', platform, num_qubits)

        k.gate("x", [0])
        k.gate("y", [0])
        k.gate("z", [0])
        k.gate("h", [0])
        k.gate("i", [0])
        k.gate("s", [0])
        k.gate("t", [0])
        k.gate("sdag", [0])
        k.gate("tdag", [0])

        k.gate('cnot', [0, 1])
        k.gate('cphase', [1, 2])
        k.gate('swap', [2, 0])

        k.gate('toffoli', [0, 1, 2])

        # generate conjugate of k
        ck.conjugate(k)

        p.add_kernel(k)
        p.add_kernel(ck)

        p.compile()

        gold_fn = os.path.join(qasm_golden_dir, 'test_conjugate.qasm')
        qasm_fn = os.path.join(output_dir, p.name + '.qasm')
        self.assertTrue(file_compare(qasm_fn, gold_fn))


if __name__ == '__main__':
    unittest.main()

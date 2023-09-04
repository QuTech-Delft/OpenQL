import openql as ql
import os
import unittest

from config import output_dir, qasm_golden_dir
from utils import file_compare


platform = ql.Platform("starmon", "none")


class TestCqasm(unittest.TestCase):
    @classmethod
    def setUp(cls):
        ql.initialize()
        ql.set_option('output_dir', output_dir)
        ql.set_option('optimize', 'no')
        ql.set_option('scheduler', 'ASAP')
        ql.set_option('log_level', 'LOG_INFO')
        # ql.set_option('write_qasm_files', 'yes')

    def test_cqasm_default_gates(self):
        ql.set_option('use_default_gates', 'yes')

        num_qubits = 4

        k = ql.Kernel("aKernel", platform, num_qubits)
        k.gate('prep_z', [0])
        k.gate('identity', [0])
        k.gate('hadamard', [0])
        k.gate('x', [0])
        k.gate('y', [0])
        k.gate('z', [0])
        k.gate('rx90', [0])
        k.gate('ry90', [0])
        k.gate('mrx90', [0])
        k.gate('mry90', [0])
        k.gate('s', [0])
        k.gate('sdag', [0])
        k.gate('t', [0])
        k.gate('tdag', [0])
        k.gate('rx', [0], 0, 0.15)
        k.gate('ry', [0], 0, 0.15)
        k.gate('rz', [0], 0, 0.15)
        k.gate('cnot', [0, 1])
        k.gate('cz', [0, 1])
        k.gate('toffoli', [0, 1, 2])
        k.gate('swap', [0, 1])
        k.gate('measure', [0])
        k.gate('measure', [1])

        p = ql.Program('test_cqasm_default_gates', platform, num_qubits)
        p.add_kernel(k)
        p.compile()

        for ext in ('.qasm', '_scheduled.qasm'):
            gold_fn = os.path.join(qasm_golden_dir, p.name + ext)
            qisa_fn = os.path.join(output_dir, p.name + ext)

            self.assertTrue(file_compare(qisa_fn, gold_fn))

    # @unittest.skip
    def test_cqasm_custom_gates(self):
        ql.set_option('use_default_gates', 'no')

        num_qubits = 4

        k = ql.Kernel("aKernel", platform, num_qubits)
        k.gate('prep_z', [0])
        k.gate('i', [0])
        k.gate('h', [0])
        k.gate('x', [0])
        k.gate('y', [0])
        k.gate('z', [0])
        k.gate('x90', [0])
        k.gate('y90', [0])
        k.gate('mx90', [0])
        k.gate('my90', [0])
        k.gate('s', [0])
        k.gate('sdag', [0])
        k.gate('t', [0])
        k.gate('tdag', [0])
        k.gate('rx', [0], 0, 0.15)
        k.gate('ry', [0], 0, 0.15)
        k.gate('rz', [0], 0, 0.15)
        k.gate('cnot', [0, 1])
        k.gate('cz', [0, 1])
        k.gate('toffoli', [0, 1, 2])
        k.gate('swap', [0, 1])
        k.gate('measure', [0])
        k.gate('measure', [1])

        p = ql.Program('test_cqasm_custom_gates', platform, num_qubits)
        p.add_kernel(k)
        p.compile()

        for ext in ('.qasm', '_scheduled.qasm'):
            gold_fn = os.path.join(qasm_golden_dir, p.name + ext)
            qisa_fn = os.path.join(output_dir, p.name + ext)

            self.assertTrue(file_compare(qisa_fn, gold_fn))


if __name__ == '__main__':
    unittest.main()

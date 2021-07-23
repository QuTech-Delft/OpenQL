import os
import filecmp
import unittest
from utils import file_compare
from openql import openql as ql

curdir = os.path.dirname(os.path.realpath(__file__))
platf = ql.Platform("starmon", "none")

output_dir = os.path.join(curdir, 'test_output')

class Test_cqasm(unittest.TestCase):

    @classmethod
    def setUp(self):
        ql.initialize()
        ql.set_option('output_dir', output_dir)
        ql.set_option('optimize', 'no')
        ql.set_option('scheduler', 'ASAP')
        ql.set_option('log_level', 'LOG_INFO')
        # ql.set_option('write_qasm_files', 'yes')


    def test_cqasm_default_gates(self):
        ql.set_option('use_default_gates', 'yes')

        nqubits = 4

        k = ql.Kernel("aKernel", platf, nqubits)
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

        sweep_points = [2]

        p = ql.Program('test_cqasm_default_gates', platf, nqubits)
        p.set_sweep_points(sweep_points)
        p.add_kernel(k)
        p.compile()

        for ext in ('.qasm', '_scheduled.qasm'):
            GOLD_fn = os.path.join(curdir, 'golden', p.name + ext)
            QISA_fn = os.path.join(output_dir, p.name + ext)

            self.assertTrue(file_compare(QISA_fn, GOLD_fn))

    # @unittest.skip
    def test_cqasm_custom_gates(self):
        ql.set_option('use_default_gates', 'no')

        nqubits = 4

        k = ql.Kernel("aKernel", platf, nqubits)
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

        sweep_points = [2]

        p = ql.Program('test_cqasm_custom_gates', platf, nqubits)
        p.set_sweep_points(sweep_points)
        p.add_kernel(k)
        p.compile()

        for ext in ('.qasm', '_scheduled.qasm'):
            GOLD_fn = os.path.join(curdir, 'golden', p.name + ext)
            QISA_fn = os.path.join(output_dir, p.name + ext)

            self.assertTrue(file_compare(QISA_fn, GOLD_fn))

if __name__ == '__main__':
    unittest.main()

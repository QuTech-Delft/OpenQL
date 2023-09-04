import openql as ql
import os
import unittest

from config import output_dir, qasm_golden_dir
from utils import file_compare


class TestBasic(unittest.TestCase):
    @classmethod
    def setUp(cls):
        ql.initialize()

        ql.set_option('output_dir', output_dir)
        ql.set_option('optimize', 'no')
        ql.set_option('log_level', 'LOG_WARNING')
        ql.set_option('use_default_gates', 'no')
        # ql.set_option('write_qasm_files', 'yes')

        # TODO cleanup
        ql.set_option('scheduler', 'ALAP')
        ql.set_option('scheduler_post179', 'yes')

    def test_compilation(self):
        print('output dir : {}'.format(ql.get_option('output_dir')))
        platform = ql.Platform("starmon", 'none')
        num_qubits = 2
        p = ql.Program("basic", platform, num_qubits, num_qubits)

        # populate kernel
        k = ql.Kernel("first_kernel", platform, num_qubits, num_qubits)

        k.gate('prep_z', [0])
        k.gate('x', [0])
        k.gate('x90', [0])
        k.gate('measure', [0])
        p.add_kernel(k)

        k = ql.Kernel("second_kernel", platform, num_qubits, num_qubits)
        k.gate('prep_z', [0])
        k.gate('x90', [0])
        k.gate('cz', [0, 1])
        k.gate('x90', [0])
        k.gate("measure", [0])
        p.add_kernel(k)

        p.compile()

        for name in ('basic.qasm', 'basic_scheduled.qasm'):
            self.assertTrue(file_compare(os.path.join(output_dir, name), os.path.join(qasm_golden_dir, name)))


if __name__ == '__main__':
    unittest.main()

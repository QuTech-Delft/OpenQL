import openql as ql
import os
import unittest

from config import json_dir, output_dir, qasm_golden_dir
from utils import file_compare


config_fn = os.path.join(json_dir, 'test_config_default.json')
platform = ql.Platform("starmon", config_fn)


class TestQubits(unittest.TestCase):
    @classmethod
    def setUp(cls):
        ql.initialize()
        ql.set_option('output_dir', output_dir)
        ql.set_option('optimize', 'no')
        ql.set_option('scheduler', 'ALAP')
        ql.set_option('log_level', 'LOG_WARNING')
        # ql.set_option('write_qasm_files', 'yes')

    def test_1_qubit(self):
        num_qubits = 1

        k = ql.Kernel("aKernel", platform, num_qubits)

        # populate kernel
        k.prepz(0)
        k.x(0)
        k.y(0)
        k.rx90(0)
        k.measure(0)

        p = ql.Program("1_qubit_program", platform, num_qubits)

        p.add_kernel(k)  # add kernel to program
        p.compile()

        gold_fn = os.path.join(qasm_golden_dir, 'test_1_qubit.qasm')
        qasm_fn = os.path.join(output_dir, p.name + '.qasm')
        self.assertTrue(file_compare(qasm_fn, gold_fn))

    def test_2_qubit(self):
        num_qubits = 3
        k = ql.Kernel("aKernel", platform, num_qubits)

        # populate kernel
        k.prepz(0)
        k.prepz(1)
        k.prepz(2)
        k.cz(0, 1)
        k.clifford(1, 2)
        k.measure(2)

        p = ql.Program("2_qubit_program", platform, num_qubits)

        p.add_kernel(k)  # add kernel to program
        p.compile()

        gold_fn = os.path.join(qasm_golden_dir, 'test_2_qubit.qasm')
        qasm_fn = os.path.join(output_dir, p.name + '.qasm')
        self.assertTrue(file_compare(qasm_fn, gold_fn))

    def test_3_qubit(self):
        num_qubits = 3

        k = ql.Kernel("aKernel", platform, num_qubits)

        # populate kernel
        k.prepz(0)
        k.prepz(1)
        k.prepz(2)
        k.toffoli(0, 1, 2)
        k.measure(2)

        p = ql.Program("3_qubit_program", platform, num_qubits)

        p.add_kernel(k)  # add kernel to program

        ql.set_option('decompose_toffoli', 'no')
        p.compile()

        gold_fn = os.path.join(qasm_golden_dir, 'test_3_qubit.qasm')
        qasm_fn = os.path.join(output_dir, p.name + '.qasm')
        self.assertTrue(file_compare(qasm_fn, gold_fn))


if __name__ == '__main__':
    unittest.main()

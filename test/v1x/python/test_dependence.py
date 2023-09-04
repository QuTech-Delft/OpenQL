import openql as ql
import os
import unittest

from config import json_dir, output_dir, qasm_golden_dir
from utils import file_compare

config_fn = os.path.join(json_dir, 'test_config_default.json')
platform = ql.Platform("starmon", config_fn)


class TestDependence(unittest.TestCase):
    def setUp(self):
        ql.initialize()
        ql.set_option('output_dir', output_dir)
        ql.set_option('optimize', 'no')
        ql.set_option('scheduler', 'ASAP')
        ql.set_option('scheduler_commute', 'no')
        ql.set_option('log_level', 'LOG_WARNING')
        # ql.set_option('write_qasm_files', 'yes')

    # @unittest.expectedFailure
    # @unittest.skip
    def test_independent(self):
        num_qubits = 4
        # populate kernel
        kernel = ql.Kernel("aKernel", platform, num_qubits)

        for i in range(4):
            kernel.prepz(i)

        # no dependence
        kernel.cz(0, 1)
        kernel.cz(2, 3)

        kernel.measure(0)
        kernel.measure(1)

        program = ql.Program("independent", platform, num_qubits)
        program.add_kernel(kernel)
        program.compile()

        gold_fn = os.path.join(qasm_golden_dir, 'test_independence.qasm')
        qasm_fn = os.path.join(output_dir, program.name + '_scheduled.qasm')
        self.assertTrue(file_compare(qasm_fn, gold_fn))

    # @unittest.skip
    def test_waw(self):
        num_qubits = 4
        # populate kernel
        kernel = ql.Kernel("aKernel", platform, num_qubits)

        for i in range(4):
            kernel.prepz(i)

        # q1 dependence
        kernel.cz(0, 1)
        kernel.cz(2, 1)

        kernel.measure(0)
        kernel.measure(1)

        program = ql.Program("WAW", platform, num_qubits)
        program.add_kernel(kernel)
        program.compile()

        gold_fn = os.path.join(qasm_golden_dir, 'test_WAW_ASAP.qasm')
        qasm_fn = os.path.join(output_dir, program.name + '_scheduled.qasm')

        self.assertTrue(file_compare(qasm_fn, gold_fn))

    # @unittest.skip
    def test_rar_control(self):
        num_qubits = 4

        # populate kernel
        kernel = ql.Kernel("aKernel", platform, num_qubits)

        for i in range(4):
            kernel.prepz(i)

        # q0 dependence
        kernel.cz(0, 1)
        kernel.cz(0, 2)

        kernel.measure(0)
        kernel.measure(1)

        program = ql.Program("RAR", platform, num_qubits)
        program.add_kernel(kernel)
        program.compile()

        gold_fn = os.path.join(qasm_golden_dir, 'test_RAR_Control_ASAP.qasm')
        qasm_fn = os.path.join(output_dir, program.name + '_scheduled.qasm')
        self.assertTrue(file_compare(qasm_fn, gold_fn))

    # @unittest.skip
    def test_raw(self):

        num_qubits = 4

        # populate kernel
        kernel = ql.Kernel("aKernel", platform, num_qubits)

        for i in range(4):
            kernel.prepz(i)

        # q1 dependence
        kernel.cz(0, 1)
        kernel.cz(1, 2)

        kernel.measure(0)
        kernel.measure(1)

        program = ql.Program("RAW", platform, num_qubits)
        program.add_kernel(kernel)
        program.compile()

        gold_fn = os.path.join(qasm_golden_dir, 'test_RAW_ASAP.qasm')
        qasm_fn = os.path.join(output_dir, program.name + '_scheduled.qasm')

        self.assertTrue(file_compare(qasm_fn, gold_fn))

    # @unittest.skip
    def test_war(self):

        num_qubits = 4

        # populate kernel
        kernel = ql.Kernel("aKernel", platform, num_qubits)

        for i in range(4):
            kernel.prepz(i)

        # q0 dependence
        kernel.cz(0, 1)
        kernel.cz(2, 0)

        kernel.measure(0)
        kernel.measure(1)

        program = ql.Program("WAR", platform, num_qubits)
        program.add_kernel(kernel)
        program.compile()

        gold_fn = os.path.join(qasm_golden_dir, 'test_WAR_ASAP.qasm')
        qasm_fn = os.path.join(output_dir, program.name + '_scheduled.qasm')

        self.assertTrue(file_compare(qasm_fn, gold_fn))

    # @unittest.skip
    def test_swap_single(self):

        num_qubits = 4

        # populate kernel
        kernel = ql.Kernel("aKernel", platform, num_qubits)

        kernel.gate("x", [0])
        kernel.gate("swap", [0, 1])
        kernel.gate("x", [0])

        program = ql.Program("swap_single", platform, num_qubits)
        program.add_kernel(kernel)
        program.compile()

        gold_fn = os.path.join(qasm_golden_dir, 'test_swap_single_ASAP.qasm')
        qasm_fn = os.path.join(output_dir, program.name + '_scheduled.qasm')
        self.assertTrue(file_compare(qasm_fn, gold_fn))

    # @unittest.skip
    def test_swap_multi(self):

        num_qubits = 5

        # populate kernel
        kernel = ql.Kernel("aKernel", platform, num_qubits)

        # swap test with 2 qubit gates
        kernel.gate("x", [0])
        kernel.gate("x", [1])
        kernel.gate("swap", [0, 1])
        kernel.gate("cz", [0, 2])
        kernel.gate("cz", [1, 4])

        program = ql.Program("swap_multi", platform, num_qubits)
        program.add_kernel(kernel)
        program.compile()

        gold_fn = os.path.join(qasm_golden_dir, 'test_swap_multi_ASAP.qasm')
        qasm_fn = os.path.join(output_dir, program.name + '_scheduled.qasm')
        self.assertTrue(file_compare(qasm_fn, gold_fn))


if __name__ == '__main__':
    unittest.main()

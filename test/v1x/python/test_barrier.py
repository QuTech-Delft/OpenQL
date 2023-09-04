import openql as ql
import os
import unittest

from config import output_dir, qasm_golden_dir
from utils import file_compare


class TestBarrier(unittest.TestCase):
    def setUp(self):
        ql.initialize()
        ql.set_option('output_dir', output_dir)
        ql.set_option('log_level', 'LOG_WARNING')
        ql.set_option('optimize', 'no')
        ql.set_option('scheduler', 'ALAP')
        ql.set_option('scheduler_post179', 'yes')
        ql.set_option("scheduler_commute", 'no')
        ql.set_option('use_default_gates', 'yes')
        # ql.set_option('write_qasm_files', 'yes')

    # barrier on specified qubits
    def test_barrier(self):
        platform = ql.Platform('seven_qubits_chip', 'cc_light')
        num_qubits = platform.get_qubit_number()
        p = ql.Program('test_barrier', platform, num_qubits)

        k = ql.Kernel('aKernel', platform, num_qubits)

        k.gate("x", [0])
        k.gate("x", [1])
        k.gate("y", [0])

        # k.barrier([0, 1])
        # OR
        k.gate("barrier", [0, 1])

        k.gate("measure", [0])
        k.gate("measure", [1])

        p.add_kernel(k)
        p.compile()

        qasm_fn = os.path.join(output_dir, p.name + '_last.qasm')
        gold_fn = os.path.join(qasm_golden_dir, 'test_barrier_last.qasm')
        self.assertTrue(file_compare(qasm_fn, gold_fn))

    # barrier on specified qubits with 'wait' and duration = 0
    def test_wait_barrier(self):
        platform = ql.Platform('seven_qubits_chip', 'cc_light')
        num_qubits = platform.get_qubit_number()
        p = ql.Program('test_wait_barrier', platform, num_qubits)

        k = ql.Kernel('aKernel', platform, num_qubits)

        k.gate("x", [0])
        k.gate("x", [1])
        k.gate("y", [0])
        k.gate("wait", [0, 1], 0)  # this will serve as barrier
        k.gate("measure", [0])
        k.gate("measure", [1])

        p.add_kernel(k)
        ql.print_options()
        p.compile()

        qasm_fn = os.path.join(output_dir, p.name + '_last.qasm')
        gold_fn = os.path.join(qasm_golden_dir, 'test_wait_barrier_last.qasm')
        self.assertTrue(file_compare(qasm_fn, gold_fn))

    # barrier on all qubits with barrier
    def test_barrier_all_1(self):
        platform = ql.Platform('seven_qubits_chip', 'cc_light')
        num_qubits = platform.get_qubit_number()
        p = ql.Program('test_barrier_all', platform, num_qubits)

        k = ql.Kernel('aKernel', platform, num_qubits)

        k.gate('ry90', [0])
        k.gate('ry90', [2])
        k.gate('rx90', [2])

        k.gate('measure', [0])
        k.gate('measure', [1])
        k.gate('measure', [2])
        k.gate('measure', [4])

        # with barrier syntax
        k.barrier()

        k.gate('ry90', [4])
        k.gate('ry90', [1])

        k.gate('measure', [0])
        k.gate('measure', [1])
        k.gate('measure', [2])
        k.gate('measure', [4])

        k.barrier()

        k.gate('ry90', [4])
        k.gate('ry90', [1])

        p.add_kernel(k)
        p.compile()

        qasm_fn = os.path.join(output_dir, p.name + '_scheduled.qasm')
        gold_fn = os.path.join(qasm_golden_dir, 'test_barrier_all.qasm')
        self.assertTrue(file_compare(qasm_fn, gold_fn))

    # barrier on all qubits with generalized gate API using 'barrier'
    def test_barrier_all_2(self):
        platform = ql.Platform('seven_qubits_chip', 'cc_light')
        num_qubits = platform.get_qubit_number()
        p = ql.Program('test_barrier_all', platform, num_qubits)

        k = ql.Kernel('aKernel', platform, num_qubits)

        k.gate('ry90', [0])
        k.gate('ry90', [2])
        k.gate('rx90', [2])

        k.gate('measure', [0])
        k.gate('measure', [1])
        k.gate('measure', [2])
        k.gate('measure', [4])

        # with generalized gate syntax
        k.gate('barrier', [])

        k.gate('ry90', [4])
        k.gate('ry90', [1])

        k.gate('measure', [0])
        k.gate('measure', [1])
        k.gate('measure', [2])
        k.gate('measure', [4])

        k.barrier()

        k.gate('ry90', [4])
        k.gate('ry90', [1])

        p.add_kernel(k)
        p.compile()

        qasm_fn = os.path.join(output_dir, p.name + '_scheduled.qasm')
        gold_fn = os.path.join(qasm_golden_dir, 'test_barrier_all.qasm')
        self.assertTrue(file_compare(qasm_fn, gold_fn))

    # barrier on all qubits with generalized gate API using wait with duration 0
    def test_barrier_all_3(self):
        platform = ql.Platform('seven_qubits_chip', 'cc_light')
        num_qubits = platform.get_qubit_number()
        p = ql.Program('test_barrier_all', platform, num_qubits)

        k = ql.Kernel('aKernel', platform, num_qubits)

        k.gate('ry90', [0])
        k.gate('ry90', [2])
        k.gate('rx90', [2])

        k.gate('measure', [0])
        k.gate('measure', [1])
        k.gate('measure', [2])
        k.gate('measure', [4])

        # with generalized gate syntax using wait with duration 0
        k.gate('wait', [], 0)

        k.gate('ry90', [4])
        k.gate('ry90', [1])

        k.gate('measure', [0])
        k.gate('measure', [1])
        k.gate('measure', [2])
        k.gate('measure', [4])

        k.barrier()

        k.gate('ry90', [4])
        k.gate('ry90', [1])

        p.add_kernel(k)
        p.compile()

        qasm_fn = os.path.join(output_dir, p.name + '_scheduled.qasm')
        gold_fn = os.path.join(qasm_golden_dir, 'test_barrier_all.qasm')
        self.assertTrue(file_compare(qasm_fn, gold_fn))


if __name__ == '__main__':
    unittest.main()

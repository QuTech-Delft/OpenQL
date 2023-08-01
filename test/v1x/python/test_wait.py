import openql as ql
import os
import unittest

from config import json_dir, output_dir, qasm_golden_dir
from utils import file_compare


class TestWait(unittest.TestCase):
    @classmethod
    def setUp(cls):
        ql.initialize()
        ql.set_option('output_dir', output_dir)
        ql.set_option('use_default_gates', 'no')
        ql.set_option('log_level', 'LOG_WARNING')
        ql.set_option('optimize', 'no')
        ql.set_option('scheduler', 'ASAP')
        ql.set_option('scheduler_post179', 'yes')
        ql.set_option("scheduler_commute", 'no')

    @unittest.skip
    def test_wait_simple(self):
        config_fn = os.path.join(json_dir, 'config_cc_light.json')
        platform = ql.Platform('seven_qubits_chip', config_fn)
        num_qubits = platform.get_qubit_number()
        p = ql.Program('test_wait_simple', platform, num_qubits)

        k = ql.Kernel('aKernel', platform, num_qubits)

        k.gate("x", [0])
        k.gate("wait", [0], 40)  # OR k.wait([0], 40)
        k.gate("x", [0])

        p.add_kernel(k)
        p.compile()

        qisa_fn = os.path.join(output_dir, p.name + '.qisa')
        gold_fn = os.path.join(qasm_golden_dir, 'test_wait_simple.qisa')
        self.assertTrue(file_compare(qisa_fn, gold_fn))

    @unittest.skip
    def test_wait_parallel(self):
        config_fn = os.path.join(json_dir, 'config_cc_light.json')
        platform = ql.Platform('seven_qubits_chip', config_fn)
        num_qubits = platform.get_qubit_number()
        p = ql.Program('test_wait_parallel', platform, num_qubits)

        k = ql.Kernel('aKernel', platform, num_qubits)

        # wait should not be in parallel with another gate
        k.gate("x", [0])
        k.gate("wait", [1], 20)  # OR k.wait([0], 20)
        k.gate("x", [1])

        p.add_kernel(k)
        p.compile()

        qisa_fn = os.path.join(output_dir, p.name + '.qisa')
        gold_fn = os.path.join(qasm_golden_dir, 'test_wait_parallel.qisa')
        self.assertTrue(file_compare(qisa_fn, gold_fn))

    @unittest.skip
    def test_wait_sweep(self):
        config_fn = os.path.join(json_dir, 'config_cc_light.json')
        platform = ql.Platform('seven_qubits_chip', config_fn)
        num_qubits = 7
        p = ql.Program('test_wait_sweep', platform, num_qubits)

        qubit_idx = 0
        waits = [20, 40, 60, 100, 200, 400, 800, 1000, 2000]
        for kno, wait_nanoseconds in enumerate(waits):
            k = ql.Kernel("kernel_"+str(kno), platform, num_qubits)

            k.prepz(qubit_idx)

            k.gate('rx90', [qubit_idx])
            k.gate("wait", [qubit_idx], wait_nanoseconds)

            k.gate('rx180', [qubit_idx])
            k.gate("wait", [qubit_idx], wait_nanoseconds)

            k.gate('rx90', [qubit_idx])
            k.gate("wait", [qubit_idx], wait_nanoseconds)

            k.gate('measure', [qubit_idx])

            # add the kernel to the program
            p.add_kernel(k)

        # compile the program
        p.compile()

        qisa_fn = os.path.join(output_dir, p.name + '.qisa')
        gold_fn = os.path.join(qasm_golden_dir, 'test_wait_sweep.qisa')
        self.assertTrue(file_compare(qisa_fn, gold_fn))

    @unittest.skip
    def test_wait_multi(self):
        config_fn = os.path.join(json_dir, 'config_cc_light.json')
        platform = ql.Platform('seven_qubits_chip', config_fn)
        num_qubits = platform.get_qubit_number()
        p = ql.Program('test_wait_multi', platform, num_qubits)

        k = ql.Kernel('aKernel', platform, num_qubits)

        for i in range(4):
            k.gate("x", [i])

        k.gate("wait", [0, 1, 2, 3], 40)
        k.wait([0, 1, 2, 3], 40)

        for i in range(4):
            k.gate("measure", [i])

        p.add_kernel(k)
        p.compile()

        qisa_fn = os.path.join(output_dir, p.name + '.qisa')
        gold_fn = os.path.join(qasm_golden_dir, 'test_wait_multi.qisa')
        self.assertTrue(file_compare(qisa_fn, gold_fn))


if __name__ == '__main__':
    unittest.main()

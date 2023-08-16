import openql as ql
import os
import unittest

from config import json_dir, output_dir


config_fn = os.path.join(json_dir, "test_config_default.json")
platform = ql.Platform("starmon", config_fn)


class TestKernel(unittest.TestCase):
    @classmethod
    def setUp(cls):
        ql.initialize()
        ql.set_option('output_dir', output_dir)
        ql.set_option('optimize', 'no')
        ql.set_option('scheduler', 'ASAP')
        ql.set_option('log_level', 'LOG_WARNING')

    def test_kernel_name(self):
        name = "kernel1"
        num_qubits = 3
        k = ql.Kernel(name, platform, num_qubits)
        self.assertEqual(k.name, name)

    def test_kernel_qubit_count(self):
        name = "kernel1"
        num_qubits = 3
        k = ql.Kernel(name, platform, num_qubits)
        self.assertEqual(k.qubit_count, num_qubits)

    def test_kernel_creg_count(self):
        name = "kernel1"
        num_qubits = 2
        num_cregs = 3
        k = ql.Kernel(name, platform, num_qubits, num_cregs)
        self.assertEqual(k.creg_count, num_cregs)

    def test_allowed_operations(self):
        num_qubits = 3
        k = ql.Kernel("kernel1", platform, num_qubits)
        # The following operations can be executed using a kernel
        operations = [
            # SPAM
            'prepz', 'measure',
            # Single qubit rotations
            'rx180', 'rx90', 'ry180', 'ry90', 'mrx90', 'mry90',
            # Single qubit cliffords
            'clifford',
            # 2 qubit gates
            'cnot', 'cphase', 'cz',
            # Theorist gates
            'identity', 'hadamard', 's', 'sdag', 't', 'tdag', 'toffoli',
            # pauli operators
            'x', 'y', 'z',
            # arbitrary rotations
            'rx', 'ry', 'rz',
            # synchronization
            'wait', 'barrier'
        ]
        # Test that these operations exist as methods of the kernel
        self.assertTrue(set(operations).issubset(dir(k)))

    def test_simple_kernel(self):
        num_qubits = 3
        k = ql.Kernel("kernel1", platform, num_qubits)
        k.prepz(0)
        k.prepz(1)
        k.x(0)
        k.y(0)
        k.cnot(0, 1)
        k.toffoli(0, 1, 2)
        k.rx90(0)
        k.clifford(2, 0)
        k.measure(2)

        # At this point it should be tested if these things have been added
        # to the qubits. However, it is not clear how to view this

    def test_multi_kernel(self):
        num_qubits = 3

        k1 = ql.Kernel("kernel1", platform, num_qubits)
        k1.prepz(0)
        k1.prepz(1)
        k1.x(0)
        k1.y(0)
        k1.cnot(0, 1)
        k1.toffoli(0, 1, 2)
        k1.rx90(0)
        k1.clifford(2, 0)
        k1.measure(2)

        k2 = ql.Kernel("kernel2", platform, num_qubits)
        k2.prepz(0)
        k2.prepz(1)
        k2.x(0)
        k2.y(0)
        k2.cnot(0, 1)
        k2.toffoli(0, 1, 2)
        k2.rx90(0)
        k2.clifford(2, 0)
        k2.measure(2)

        p = ql.Program("test_multi_kernel", platform, num_qubits)
        p.add_kernel(k1)
        p.add_kernel(k2)

        p.compile()

    def test_duplicate_kernel_name(self):
        num_qubits = 3

        p = ql.Program("test_duplicate_kernel_name", platform, num_qubits)
        k1 = ql.Kernel("aKernel1", platform, num_qubits)
        k2 = ql.Kernel("aKernel2", platform, num_qubits)
        k3 = ql.Kernel("aKernel1", platform, num_qubits)

        k1.gate('x', [0])
        k2.gate('x', [0])
        k3.gate('x', [0])

        # add the kernel to the program
        p.add_kernel(k1)
        p.add_kernel(k2)

        # following call to add_kernel should fail as k3 has duplicate name
        try:
            p.add_kernel(k3)
            raise
        except:
            pass

        # compile the program
        p.compile()


if __name__ == '__main__':
    unittest.main()

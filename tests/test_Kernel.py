import os
import unittest
from openql import openql as ql

curdir = os.path.dirname(os.path.realpath(__file__))
config_fn = os.path.join(curdir, 'test_config_default.json')
platf = ql.Platform("starmon", config_fn)

output_dir = os.path.join(curdir, 'test_output')

class Test_kernel(unittest.TestCase):

    @classmethod
    def setUp(self):
        ql.initialize()
        ql.set_option('output_dir', output_dir)
        ql.set_option('optimize', 'no')
        ql.set_option('scheduler', 'ASAP')
        ql.set_option('log_level', 'LOG_WARNING')

    def test_kernel_name(self):
        name = "kernel1"
        nqubits = 3
        k = ql.Kernel(name, platf, nqubits)
        self.assertEqual(k.name, name)

    def test_kernel_qubit_count(self):
        name = "kernel1"
        nqubits=3
        k = ql.Kernel(name, platf, nqubits)
        self.assertEqual(k.qubit_count, nqubits)

    def test_kernel_creg_count(self):
        name = "kernel1"
        nqubits=2
        ncreg = 3
        k = ql.Kernel(name, platf, nqubits, ncreg)
        self.assertEqual(k.creg_count, ncreg)


    def test_allowed_operations(self):
        nqubits = 3
        k = ql.Kernel("kernel1", platf, nqubits)
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
        nqubits = 3
        k = ql.Kernel("kernel1", platf, nqubits)
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
        sweep_points = [2]
        nqubits = 3

        k1 = ql.Kernel("kernel1", platf, nqubits)
        k1.prepz(0)
        k1.prepz(1)
        k1.x(0)
        k1.y(0)
        k1.cnot(0, 1)
        k1.toffoli(0, 1, 2)
        k1.rx90(0)
        k1.clifford(2, 0)
        k1.measure(2)

        k2 = ql.Kernel("kernel2", platf, nqubits)
        k2.prepz(0)
        k2.prepz(1)
        k2.x(0)
        k2.y(0)
        k2.cnot(0, 1)
        k2.toffoli(0, 1, 2)
        k2.rx90(0)
        k2.clifford(2, 0)
        k2.measure(2)

        p = ql.Program("test_multi_kernel", platf, nqubits)
        p.set_sweep_points(sweep_points)
        p.add_kernel(k1)
        p.add_kernel(k2)

        p.compile()

    def test_duplicate_kernel_name(self):
        nqubits = 3

        p = ql.Program("test_duplicate_kernel_name", platf, nqubits)
        k1 = ql.Kernel("aKernel1", platf, nqubits)
        k2 = ql.Kernel("aKernel2", platf, nqubits)
        k3 = ql.Kernel("aKernel1", platf, nqubits)

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

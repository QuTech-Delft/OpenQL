import unittest
from openql import Kernel


class Test_kernel(unittest.TestCase):

    @classmethod
    def setUpClass(self):
        pass

    def test_allowed_operations(self):
        k = Kernel("kernel1")
        # The following operations can be executed using a kernel
        operations = [
            # SPAM
            'prepz', 'measure',
            # Single qubit rotations
            'rx180', 'rx90', 'ry180', 'ry90', 'mrx90', 'mry90',
            # Single qubit cliffords
            'clifford',
            # 2 qubit gates
            'cnot', 'cphase',
            # Theorist gates
            'hadamard', 's', 'sdag', 'toffoli',
            # pauli operators
            'x', 'y', 'z']
        # Test that these operations exist as methods of the kernel
        self.assertTrue(set(operations).issubset(dir(k)))

    @unittest.skip
    def test_kernel_name(self):
        name = "kernel1"
        k = Kernel(name)
        # kernel does not have a name attribute at this moment
        self.assertEqual(k.name, name)

    def test_simple_kernel(self):
        k = Kernel("kernel1")
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

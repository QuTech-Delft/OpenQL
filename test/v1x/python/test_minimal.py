import openql as ql
import unittest

from config import output_dir


platform = ql.Platform("starmon", "cc_light")


class TestKernel(unittest.TestCase):
    @classmethod
    def setUp(cls):
        ql.initialize()
        ql.set_option('output_dir', output_dir)

    def test_minimal(self):
        num_qubits = 1

        # create a kernel
        k = ql.Kernel("aKernel", platform, num_qubits)

        # populate a kernel
        k.prepz(0)
        k.identity(0)
        k.measure(0)

        # create a program
        p = ql.Program("minimal", platform, num_qubits)

        # add kernel to program
        p.add_kernel(k)

        # compile  the program
        p.compile()
        # all the outputs are generated in 'output' dir


if __name__ == '__main__':
    unittest.main()

import os
import unittest
from openql import openql as ql

curdir = os.path.dirname(os.path.realpath(__file__))
config_fn = os.path.join(curdir, 'hardware_config_cc_light.json')
platf = ql.Platform("starmon", config_fn)

output_dir = os.path.join(curdir, 'test_output')


class Test_kernel(unittest.TestCase):

    @classmethod
    def setUp(self):
        ql.initialize()
        ql.set_option('output_dir', output_dir)

    def minimal(self):
        nqubits = 1

        # create a kernel
        k = ql.Kernel("aKernel", platf, nqubits)

        # populate a kernel
        k.prepz(0)
        k.identity(0)
        k.measure(0)

        sweep_points = [2]

        # create a program
        p = ql.Program("minimal", platf, nqubits)
        p.set_sweep_points(sweep_points, len(sweep_points))

        # add kernel to program
        p.add_kernel(k)

        # compile  the program
        p.compile()
        # all the outputs are generated in 'output' dir

if __name__ == '__main__':
    unittest.main()

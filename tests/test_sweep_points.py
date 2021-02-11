import os
import unittest
import json
from openql import openql as ql

curdir = os.path.dirname(os.path.realpath(__file__))
config_fn = os.path.join(curdir, 'hardware_config_cc_light.json')
platf = ql.Platform("starmon", config_fn)
output_dir = os.path.join(curdir, 'test_output')

class Test_sweep_points(unittest.TestCase):

    @classmethod
    def setUp(self):
        ql.initialize()
        ql.set_option('output_dir', output_dir)
        ql.set_option('optimize', 'no')
        ql.set_option('scheduler', 'ALAP')
        ql.set_option('log_level', 'LOG_WARNING')
            
    def test_sweep_points(self):
        sweep_points = [0.25, 1, 1.5, 2, 2.25]
        nqubits = 1

        # create a kernel
        k = ql.Kernel("aKernel", platf, nqubits)

        # populate a kernel
        k.prepz(0)
        k.measure(0)

        # create a program
        p = ql.Program("test_sweep_points", platf, nqubits)
        p.set_sweep_points(sweep_points)

        # add kernel to program
        p.add_kernel(k)

        # compile
        p.compile()

        # all the outputs are generated in 'output' dir
        with open(os.path.join(output_dir, 'test_sweep_points_config.json')) as fp:
            config = json.load(fp)

        self.assertTrue('measurement_points' in config.keys())

        # compare list in json with initial list of sweep_points
        matched = [(i, j) for i, j in zip(config['measurement_points'], sweep_points)]
        self.assertEqual(len(matched), len(sweep_points))

    def test_no_sweep_points(self):
        nqubits = 1

        # create a kernel
        k = ql.Kernel("aKernel", platf, nqubits)

        # populate a kernel
        k.prepz(0)
        k.measure(0)

        # create a program
        p = ql.Program("test_no_sweep_points", platf, nqubits)

        # add kernel to program
        p.add_kernel(k)

        # compile
        p.compile()


if __name__ == '__main__':
    unittest.main()

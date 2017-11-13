import os
import unittest
import json
from openql import openql as ql


class Test_sweep_points(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        curdir = os.path.dirname(__file__)
        config_fn = os.path.join(curdir, 'test_cfg_cbox.json')
        cls.platf = ql.Platform("starmon", config_fn)

        cls.output_dir = os.path.join(curdir, 'test_output')
        ql.set_output_dir(cls.output_dir)

            
    def test_sweep_points(self):
        # create a kernel
        k = ql.Kernel("aKernel", self.__class__.platf)

        # populate a kernel
        k.prepz(0)
        k.identity(0)
        k.measure(0)

        sweep_points = [0.25, 1, 1.5, 2, 2.25]
        num_circuits = 1
        nqubits = 1

        # create a program
        p = ql.Program("aProgram", nqubits, self.__class__.platf)
        p.set_sweep_points(sweep_points, len(sweep_points))

        # add kernel to program
        p.add_kernel(k)

        # compile  opt  verbose
        p.compile(False, "ALAP", False)

        # all the outputs are generated in 'output' dir
        with open(os.path.join(self.__class__.output_dir, 'aProgram_config.json')) as fp:
            config = json.load(fp)

        self.assertTrue('measurement_points' in config.keys())

        # compare list in json with initial list of sweep_points
        matched = [(i, j) for i, j in zip(config['measurement_points'], sweep_points)]
        self.assertEqual(len(matched), len(sweep_points))

if __name__ == '__main__':
    unittest.main()

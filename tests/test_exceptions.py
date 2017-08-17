from openql import openql as ql
import unittest
import os

curdir = os.path.dirname(__file__)

class Test_basic(unittest.TestCase):

    def test_config_exception(self):
        output_dir = os.path.join(curdir, 'test_output')
        ql.set_output_dir(output_dir)
        print('output dir : {}'.format( ql.get_output_dir() ) )
        config_fn = os.path.join(curdir, 'test_cfg_cbox_broken.json')
        platf = ql.Platform("starmon", config_fn)
        sweep_points = [1]
        num_circuits = 2
        nqubits = 2
        p = ql.Program("aProgram", nqubits, platf)
        p.set_sweep_points(sweep_points, num_circuits)

        # populate kernel
        k = ql.Kernel("first_kernel", platf)
        k.gate("prepz", 0)
        k.gate("measure", 0)
        p.add_kernel(k)

        p.compile(optimize=False, verbose=False)

if __name__ == '__main__':
    unittest.main()

from openql import openql as ql
import unittest
import os

curdir = os.path.dirname(__file__)

class Test_basic(unittest.TestCase):

    def test_compilation(self):
        # set global options kernel
        output_dir = os.path.join(curdir, 'test_output')
        ql.set_output_dir(output_dir)
        print('output dir : {}'.format( ql.get_output_dir() ) )
        config_fn = os.path.join(curdir, 'test_cfg_cbox.json')
        platf = ql.Platform("starmon", config_fn)
        sweep_points = [1]
        num_circuits = 2
        nqubits = 2
        p = ql.Program("aProgram", nqubits, platf)
        p.set_sweep_points(sweep_points, len(sweep_points))

        # populate kernel
        k = ql.Kernel("first_kernel", platf)
        # k.prepz(0)
        k.gate("prepz", 0)
        # k.x(0)
        k.gate('rx180', 0)
        k.gate("measure", 0)
        p.add_kernel(k)

        k = ql.Kernel("second_kernel", platf)
        # k.prepz(0)
        k.gate("prepz", 0)
        k.gate('rx90', 0)
        k.gate('rx180', 1)
        k.gate('cz', [0, 1])
        k.gate('rx90', 0)
        k.gate("measure", 0)
        p.add_kernel(k)

        p.compile(False, "ALAP", True)


if __name__ == '__main__':
    unittest.main()

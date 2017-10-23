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
        p.set_sweep_points(sweep_points, num_circuits)

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

        p.compile(optimize=False, verbose=False)

        # N.B. the output file get's created in the output directory
        # next to where this file was called.


    # def test_scheduling(self):
    #     # set global options kernel
    #     ql.set_instruction_map_file("instructions.map")
    #     ql.init()

    #     # populate kernel
    #     k = ql.Kernel("aKernel")
    #     k.prepz(0)
    #     k.prepz(1)
    #     k.identity(0)
    #     k.identity(1)
    #     k.measure(0)
    #     k.measure(1)

    #     sweep_points = [2]
    #     num_circuits = 1
    #     nqubits = 2

    #     p = ql.Program("aProgram", nqubits)
    #     p.set_sweep_points(sweep_points, num_circuits)
    #     p.add_kernel(k)
    #     p.compile(False, True)
    #     p.schedule()


if __name__ == '__main__':
    unittest.main()

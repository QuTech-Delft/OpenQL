from openql import openql as ql
import unittest
import os

curdir = os.path.dirname(__file__)


class Test_basic(unittest.TestCase):

    def test_compilation(self):
        # set global options kernel
        ql.init()
        config_fn = os.path.join(curdir, 'hardware_config_cbox.json')
        platf = ql.Platform("starmon", config_fn)
        k = ql.Kernel("aKernel", platf)

        # populate kernel
        # k.prepz(0)
        k.gate("prepz",0)
        # k.prepz(1)
        # k.identity(0)
        k.gate('x', 0)
        # k.identity(1)
        # k.measure(0)
        # k.measure(1)
        k.gate("measure", 0)

        sweep_points = [1]
        num_circuits = 1
        nqubits = 1

        p = ql.Program("aProgram", nqubits, platf)
        p.set_sweep_points(sweep_points, num_circuits)
        p.add_kernel(k)
        print('*'*80)
        print('*'*80)
        print('*'*80)

        p.schedule()
        p.compile(optimize=False, verbose=False)


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

import os
import unittest
from openql import openql as ql

curdir = os.path.dirname(__file__)
config_fn = os.path.join(curdir, 'test_cfg_cbox.json')
platf = ql.Platform("starmon", config_fn)

output_dir = os.path.join(curdir, 'test_output')
ql.set_output_dir(output_dir)


class Test_kernel(unittest.TestCase):

    def custom_gate_test(self):
        # create a kernel
        k = ql.Kernel("aKernel", platf)

        # load custom instruction definition
        k.load_custom_instructions("instructions.json")

        # print user-defined instructions (qasm/microcode)
        k.print_custom_instructions()

        # populate a kernel
        k.prepz(0)
        k.x(0)

        k.gate("rx180", [0])
        # or
        k.gate("rx180", 0)

        num_circuits = 1
        sweep_points = [1, 1.25, 1.75, 2.25, 2.75 ]
        nqubits = 1

        # create a program
        p = ql.Program("aProgram", nqubits, platf)
        p.set_sweep_points(sweep_points, len(sweep_points))

        # add kernel to program
        p.add_kernel(k)

        # compile  opt  verbose
        p.compile(False, True)

        # schedule
        p.schedule()

        # all the outputs are generated in 'output' dir

if __name__ == '__main__':
    unittest.main()

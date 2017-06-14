import unittest
from openql import openql as ql

class Test_kernel(unittest.TestCase):

    def minimal(self):
        # set global options kernel
        ql.set_instruction_map_file("instructions.map")
        ql.init()

        # create a kernel
        k = ql.Kernel("aKernel")

        # populate a kernel
        k.prepz(0)
        k.identity(0)
        k.measure(0)

        sweep_points = [2]
        num_circuits = 1
        nqubits = 1

        # create a program
        p = ql.Program("aProgram", nqubits)
        p.set_sweep_points(sweep_points, num_circuits)

        # add kernel to program
        p.add_kernel(k)

        # compile  opt  verbose
        p.compile(False, True)

        # schedule
        p.schedule()

        # all the outputs are generated in 'output' dir

if __name__ == '__main__':
    unittest.main()
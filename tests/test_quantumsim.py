import os
import unittest
from openql import openql as ql

curdir = os.path.dirname(os.path.realpath(__file__))
output_dir = os.path.join(curdir, 'test_output')

class Test_quantumsim(unittest.TestCase):

    @classmethod
    def setUp(self):
        ql.initialize()
        ql.set_option('output_dir', output_dir)
        ql.set_option('optimize', 'no')
        ql.set_option('scheduler', 'ASAP')
        ql.set_option('log_level', 'LOG_WARNING')
        ql.set_option('quantumsim', 'yes')


    def test_quantumsim(self):
        # You can specify a config location, here we use a default config
        config_fn = os.path.join(curdir, 'test_mapper_s7.json')
        platform = ql.Platform('platform_quantumsim', config_fn)
        num_qubits = 3
        p = ql.Program('test_quantumsim', platform, num_qubits)
        sweep_points = [1, 2]
        p.set_sweep_points(sweep_points)

        # create a kernel
        k = ql.Kernel('aKernel', platform, num_qubits)

        # populate kernel using default gates
        k.gate("h",[0])
        k.gate("h",[2])
        k.gate("cz", [0, 2])
        k.gate("measure", [0])
        k.gate("measure", [2])

        # add the kernel to the program
        p.add_kernel(k)

        # compile the program
        p.compile()


if __name__ == '__main__':
    unittest.main()

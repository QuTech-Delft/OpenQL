import os
import unittest
from openql import openql as ql

rootDir = os.path.dirname(os.path.realpath(__file__))
curdir = os.path.dirname(__file__)
output_dir = os.path.join(curdir, 'test_output')

class Test_quantumsim(unittest.TestCase):

    @classmethod
    def setUpClass(self):
        ql.set_option('output_dir', output_dir)
        ql.set_option('optimize', 'no')
        ql.set_option('scheduler', 'ASAP')
        ql.set_option('log_level', 'LOG_WARNING')


    def test(self):
        # You can specify a config location, here we use a default config
        config_fn = os.path.join(curdir, 'test_cfg_quantumsim.json')
        platform = ql.Platform('platform_quantumsim', config_fn)
        num_qubits = 2
        p = ql.Program('aProgram', platform, num_qubits)
        sweep_points = [1, 2]
        p.set_sweep_points(sweep_points)

        # create a kernel
        k = ql.Kernel('aKernel', platform, num_qubits)

        # populate kernel using default gates
        k.gate("hadamard",[0])
        k.gate("hadamard",[1])
        k.gate("cphase", [0, 1])
        k.gate("measure", [0])
        k.gate("measure", [1])

        # add the kernel to the program
        p.add_kernel(k)

        # compile the program
        p.compile()


if __name__ == '__main__':
    unittest.main()

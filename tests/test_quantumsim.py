import os
import unittest
from openql import openql as ql

rootDir = os.path.dirname(os.path.realpath(__file__))

curdir = os.path.dirname(__file__)
output_dir = os.path.join(curdir, 'test_output')
ql.set_output_dir(output_dir)


class Test_quantumsim(unittest.TestCase):

    def test(self):
        # You can specify a config location, here we use a default config
        config_fn = os.path.join(curdir, 'test_cfg_quantumsim.json')
        platform = ql.Platform('platform_quantumsim', config_fn)
        num_qubits = 2
        p = ql.Program('aProgram', num_qubits, platform)
        sweep_points = [1, 2]
        p.set_sweep_points(sweep_points, len(sweep_points))

        # create a kernel
        k = ql.Kernel('aKernel', platform)

        # populate kernel using default gates
        k.gate("hadamard",0)
        k.gate("hadamard",1)
        k.gate("cphase", [0, 1])
        k.gate("measure", 0)
        k.gate("measure", 1)

        # add the kernel to the program
        p.add_kernel(k)

        # compile the program
        p.compile(optimize=False, scheduler='ASAP', log_level='LOG_WARNING')


if __name__ == '__main__':
    unittest.main()

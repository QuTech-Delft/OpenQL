import os
import filecmp
import unittest
import numpy as np
from openql import openql as ql

rootDir = os.path.dirname(os.path.realpath(__file__))

curdir = os.path.dirname(__file__)
config_fn = os.path.join(curdir, 'test_config_default.json')
platf = ql.Platform("starmon", config_fn)

output_dir = os.path.join(curdir, 'test_output')
ql.set_option('output_dir', output_dir)

class Test_bugs(unittest.TestCase):

    # @unittest.expectedFailure
    # @unittest.skip
    def test_typecast(self):
        sweep_points = [1,2]
        num_circuits = 1
        num_qubits = 2
        p = ql.Program('test_bug', num_qubits, platf)
        p.set_sweep_points(sweep_points, len(sweep_points))
        k = ql.Kernel('kernel1', platf)

        qubit = 1

        k.identity(np.int(qubit))
        k.identity(np.int32(qubit))
        k.identity(np.int64(qubit))

        k.identity(np.uint(qubit))
        k.identity(np.uint32(qubit))
        k.identity(np.uint64(qubit))

        # add the kernel to the program
        p.add_kernel(k)


if __name__ == '__main__':
    unittest.main()

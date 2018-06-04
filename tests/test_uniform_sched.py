import os
import unittest
from openql import openql as ql
import numpy as np

rootDir = os.path.dirname(os.path.realpath(__file__))
curdir = os.path.dirname(__file__)
output_dir = os.path.join(curdir, 'test_output')

class Test_uniform_scheduler(unittest.TestCase):

    def test_uniform_scheduler_01(self):
        ql.set_option('output_dir', output_dir)
        ql.set_option('optimize', 'no')
        ql.set_option("scheduler", "UNIFORM");
        ql.set_option('log_level', 'LOG_WARNING')

        config_fn = os.path.join(curdir, 'test_cfg_none_s7.json')
        platform  = ql.Platform('starmon', config_fn)
        num_qubits = 7
        p = ql.Program('test_uniform_scheduler_01', num_qubits, platform)
        k = ql.Kernel('kernel1', platform)

        for j in range(7):
            k.gate("x", [j])

        k.gate("cnot", [0,2])

        for j in range(7):
            k.gate("x", [j])

        k.gate("cnot", [6,3])

        for j in range(7):
            k.gate("x", [j])

        k.gate("cnot", [1,4])

        p.add_kernel(k)
        p.compile()

if __name__ == '__main__':
    unittest.main()

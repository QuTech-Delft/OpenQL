import os
import unittest
from openql import openql as ql
import numpy as np

rootDir = os.path.dirname(os.path.realpath(__file__))
curdir = os.path.dirname(__file__)
output_dir = os.path.join(curdir, 'test_output')

class Test_mapper(unittest.TestCase):

    def test_mapper_0(self):
        ql.set_option('output_dir', output_dir)
        ql.set_option('mapper', 'base')
        ql.set_option('scheduler', 'ASAP')
        ql.set_option('log_level', 'LOG_WARNING')

        config_fn = os.path.join(curdir, 'test_cfg_none_s7.json')
        platform  = ql.Platform('starmon', config_fn)

        num_qubits = 7
        p = ql.Program('test_mapper_0', num_qubits, platform)
        k = ql.Kernel('kernel_0', platform)

        # a simple first test
        # a list of all cnots that are ok in trivial mapping
        # so no swaps are inserted and map is not changed

        for j in range(7):
            k.gate("x", [j])

        k.gate("cnot", [0,2])
        k.gate("cnot", [0,3])
        k.gate("cnot", [1,3])
        k.gate("cnot", [1,4])
        k.gate("cnot", [2,0])
        k.gate("cnot", [2,5])
        k.gate("cnot", [3,0])
        k.gate("cnot", [3,1])
        k.gate("cnot", [3,5])
        k.gate("cnot", [3,6])
        k.gate("cnot", [4,1])
        k.gate("cnot", [4,6])
        k.gate("cnot", [5,2])
        k.gate("cnot", [5,3])
        k.gate("cnot", [6,3])
        k.gate("cnot", [6,4])

        p.add_kernel(k)
        p.compile()
        ql.set_option('mapper', 'no')

    def test_mapper_1(self):
        ql.set_option('output_dir', output_dir)
        ql.set_option('mapper', 'base')
        ql.set_option('optimize', 'no')
        ql.set_option('scheduler', 'ASAP')
        ql.set_option('log_level', 'LOG_WARNING')

        config_fn = os.path.join(curdir, 'test_cfg_none_s7.json')
        platform  = ql.Platform('starmon', config_fn)

        num_qubits = 7
        p = ql.Program('test_mapper_1', num_qubits, platform)
        k = ql.Kernel('kernel_1', platform)

        # one cnot with operands that are at distance 4 in s7
        # this means that 3 swaps are inserted before that cnot

        for j in range(7):
            k.gate("x", [j])

        k.gate("cnot", [2,4])

        for j in range(7):
            k.gate("x", [j])

        p.add_kernel(k)
        p.compile()
        ql.set_option('mapper', 'no')

    def test_mapper_2(self):
        ql.set_option('output_dir', output_dir)
        ql.set_option('mapper', 'base')
        ql.set_option('optimize', 'no')
        ql.set_option('scheduler', 'ASAP')
        ql.set_option('log_level', 'LOG_WARNING')

        config_fn = os.path.join(curdir, 'test_cfg_none_s7.json')
        platform  = ql.Platform('starmon', config_fn)

        num_qubits = 7
        p = ql.Program('test_mapper_2', num_qubits, platform)
        k = ql.Kernel('kernel_2', platform)

        # one cnot between each pair of qubits in s7
        # will generate a lot of swaps

        for j in range(7):
            k.gate("x", [j])

        for i in range(7):
            for j in range(7):
                if (i != j):
                    k.gate("cnot", [i,j])

        for j in range(7):
            k.gate("x", [j])

        p.add_kernel(k)
        p.compile()
        ql.set_option('mapper', 'no')

if __name__ == '__main__':
    unittest.main()
